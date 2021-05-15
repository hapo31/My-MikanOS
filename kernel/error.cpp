#include "error.hpp"

std::string CreateErrorMessage(const std::string &message, const Error &err) {
  return message + ":" + err.Name() + " at " + err.File() + ":" +
         std::to_string(err.Line()) + "\n";
}
