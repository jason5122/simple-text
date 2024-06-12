# TODO

## Priority

### OpenGL

- Ensure OpenGL function loading is context dependent
  - Take inspiration from datenwolf's `struct` approach [here](https://www.reddit.com/r/opengl/comments/17mq767/comment/k7mox6f/)

## Unordered

### Linux

- Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.
- Monitor GTK 4 slow startup issue.
  - [GNOME issue](https://gitlab.gnome.org/GNOME/gtk/-/issues/4112)
  - [Mesa issue](https://gitlab.freedesktop.org/mesa/mesa/-/issues/5113#note_2393235)

### General

- Measure performance.
