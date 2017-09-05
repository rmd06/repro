workspace(name = "repro")

load("//tools:github.bzl", "github_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

github_archive(
    name = "eigen",
    repository = "RobotLocomotion/eigen-mirror",
    commit = "d3ee2bc648be3d8be8c596a9a0aefef656ff8637",
    build_file = "tools/eigen.BUILD",
    sha256 = "db797e2857d3d6def92ec2c46aa04577d3e1bb371d6fe14e6bdfc088dcaf2e9e",
)

load("//tools:python.bzl", "python_repository")
python_repository(
    name = "python",
    version = "2.7",
)

load("//tools:numpy.bzl", "numpy_repository")
numpy_repository(
    name = "numpy",
    python_version = "2.7",
)

new_local_repository(
    name = "pybind11",
    path = "externals/pybind11",
    build_file = "tools/pybind11.BUILD",
)
# github_archive(
#     name = "pybind11",
#     repository = "RobotLocomotion/pybind11",
#     commit = "6d72785766558047ee2e2075198c07d8c25eb631",
#     build_file = "tools/pybind11.BUILD",
#     sha256 = "08b4813b3b17f607efc4e8ba8b73bf55759ba744cab125e9fc666b5161cb1d0a",
# )

github_archive(
    name = "gtest",
    repository = "google/googletest",
    commit = "release-1.8.0",
    sha256 = "58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8",  # noqa
    build_file = "tools/gtest.BUILD",
)

# Jerry-rig VTK
load("//tools:vtk.bzl", "vtk_repository")
vtk_repository(
    name = "vtk",
)

# # Jerry-rig PCL via pkg_config
# local_repository(
#     name = "io_kythe",
#     path = "externals/kythe",
# )
# load("@io_kythe//tools/build_rules/config:pkg_config.bzl", "pkg_config_package")

# # Cannot have two pkg_config packages with overlapping paths
# pkg_config_package(
#     name = "pcl_io",
#     modname = "pcl_io-1.8",
# )
# pkg_config_package(
#     name = "pcl_filters",
#     modname = "pcl_filters-1.8",
# )

# pkg_config_package(
#     name = "pcl",
#     modname = "pcl_custom-1.8",
# )
