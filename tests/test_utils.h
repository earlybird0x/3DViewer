// tests/test_utils.h
#pragma once
#include <fstream>
#include <string>

#include "model/obj_model.h"
#include "model/obj_parser.h"

inline std::string WriteTempObj(const std::string &content,
                                const std::string &name) {
  std::ofstream f(name, std::ios::binary);
  f << content;
  return name;
}

inline bool LoadModelFromObjString(const std::string &obj, s21::Model &out,
                                   std::string *err = nullptr,
                                   const std::string &name = "tmp_test.obj") {
  std::string path = WriteTempObj(obj, name);
  s21::ObjParser parser;
  return parser.Load(path, out, err);
}
