# juce-vst3-cmake-template

This is a template repo for creating VST3 plugins with JUCE using CMake as the project backend.
It's meant to simply be cloned and edited for whatever plugin you want to develop.

There are comments throughout the code explaining how the project is structured and what code should
go where.

JUCE is included as a **Git Submodule**, so clone the repo with the following command:

```bash
$ git clone https://github.com/jakerieger/juce-vst3-cmake-template <ProjectName> --recurse-submodules -j8
```