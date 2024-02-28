cmake_minimum_required(VERSION 3.25)

project(tcp_cat C)

include(bare)

add_bare_module(tcp_cat)

target_sources(
  ${tcp_cat}
  PRIVATE
    binding.c
)