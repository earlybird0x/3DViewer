#ifndef S21_OBJ_PARSER_H
#define S21_OBJ_PARSER_H

#include <string>

#include "model/obj_model.h"

namespace s21
{

    class IModelLoader
    {
    public:
        virtual ~IModelLoader() = default;

        virtual bool Load(const std::string &path,
                          Model &out,
                          std::string *err = nullptr) = 0;
    };

    class ObjParser : public IModelLoader
    {
    public:
        bool Load(const std::string &path,
                  Model &out,
                  std::string *err = nullptr) override;
    };

} // namespace s21

#endif // S21_OBJ_PARSER_H
