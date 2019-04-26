#include <taichi/visual/gui.h>
#include "../tlang.h"

TLANG_NAMESPACE_BEGIN

auto ray_march = [] {
  CoreState::set_trigger_gdb_when_crash(true);
  int n = 512;
  Program prog(Arch::gpu);
  prog.config.print_ir = true;

  Global(color_r, f32);
  Global(color_g, f32);
  Global(color_b, f32);

  layout([&]() { root.dense(0, n * n * 2).place(color_r, color_g, color_b); });

  auto sdf = [&](Vector p_) {
    float alpha = -0.7f;
    Vector p(3);
    p(0) = (cos(alpha) * p_(0) + sin(alpha) * p_(2) + 1.0_f) % 2.0_f - 1.0_f;
    p(1) = p_(1);
    p(2) = -sin(alpha) * p_(0) + cos(alpha) * p_(2);

    auto dist_sphere = p.norm() - 0.5_f;
    auto dist_walls = min(p(2) + 6.0f, p(1) + 1.0f);
    Vector d(3);
    d(0) = abs(p(0) - 1.0_f) - 0.3_f;
    d(1) = abs(p(1) + 0.5_f) - 1.2_f;
    d(2) = abs(p(2) - 1.0_f) - 0.2_f;
    auto dist_cube = norm(d.map([](const Expr &v) { return max(v, 0.0f); })) +
                     min(max(max(d(0), d(1)), d(2)), 0.0_f);
    return min(dist_sphere, min(dist_walls, dist_cube));
  };

  float32 eps = 1e-5f;
  float32 dist_limit = 1e2;
  int limit = 200;

  auto ray_march = [&](Vector p, Vector dir) {
    Local(j) = 0;
    Local(dist) = 0.0f;

    While(j < limit && sdf(p + dist * dir) > eps && dist < dist_limit, [&] {
      dist += sdf(p + dist * dir);
      j += 1;
    });
    return dist;
  };

  auto normal = [&](Vector p) {
    float d = 1e-3f;
    Vector n(3);
    for (int i = 0; i < 3; i++) {
      Vector inc = p, dec = p;
      Mutable(inc);
      Mutable(dec);
      inc(i) += d;
      dec(i) -= d;
      n(i) = (0.5f / d) * (sdf(inc) - sdf(dec));
    }
    return normalized(n);
  };

  auto out_dir = [&](Vector n) {
    Vector u({1.0f, 0.0f, 0.0f}), v(3);
    Mutable(u);
    If(abs(n(1)) < 1 - 1e-3f, [&] {
      u = normalized(cross(n, Vector({0.0f, 1.0f, 0.0f})));
    });
    v = cross(n, u);
    Local(phi) = 2 * pi * Rand<float>();
    Local(r) = Rand<float>();
    Local(alpha) = 0.5_f * pi * (r * r);
    return sin(alpha) * (cos(phi) * u + sin(phi) * v) + cos(alpha) * n;
  };

  auto background = [](Vector dir) {
    return 1.0f * max(dir(1) + dir(0), 0.0f);
  };

  float fov = 0.3;

  auto &main = kernel([&]() {
    Parallelize(8);
    Vectorize(8);
    For(0, n * n * 2, [&](Expr i) {
      Vector orig({0.0f, 0.0f, 12.0f}), c(3);
      Mutable(orig);

      c(0) = fov * (cast<float>(i / n) / float(n / 2) - 2.0f);
      c(1) = fov * (cast<float>(i % n) / float(n / 2) - 1.0f);
      c(2) = -1.0f;

      Mutable(c);
      c = normalized(c);

      Vector color(3);
      color = Vector({1.0f, 1.0f, 1.0f});
      Mutable(color);
      int depth_limit = 4;
      Local(depth) = 0;

      While(depth < depth_limit, [&] {
        depth += 1;
        Local(_dist) = ray_march(orig, c);
        If(_dist < dist_limit,
           [&] {
             orig += _dist * c;
             Vector nor;
             nor = normal(orig);
             c = normalized(out_dir(nor));
             orig += 0.01f * c;
             color *= 0.7_f;
           })
            .Else([&] {
              color = color * background(c);
              depth = depth_limit;
            });
      });

      color_r[i] += color(0);
      color_g[i] += color(1);
      color_b[i] += color(2);
    });
  });

  /// TC_P(measure_cpe(func, 1));

  GUI gui("ray march", Vector2i(n * 2, n));

  auto tone_map = [](real x) { return x; };
  constexpr int N = 1;
  for (int frame = 0;; frame++) {
    for (int i = 0; i < N; i++)
      main();
    real scale = 1.0_f / ((frame + 1) * N);
    for (int i = 0; i < n * n * 2; i++) {
      gui.buffer[i / n][i % n] =
          Vector4(tone_map(scale * color_r.val<float>(i)),
                  tone_map(scale * color_g.val<float>(i)),
                  tone_map(scale * color_b.val<float>(i)), 1);
    }
    gui.update();
  }
};
TC_REGISTER_TASK(ray_march);

TLANG_NAMESPACE_END
