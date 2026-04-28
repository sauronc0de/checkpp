#include <iostream>

// Static
static int VarASiablePatata = 42;

// Global
int volatile any_asdlfdsf = 0; // company-global-function-module-prefix

// Macros
#define MACRO_PATATA 100
#define noMACRO_PATATA 200 // company-macro-upper-case

// Enums
enum EnumPatata
{
  Value1, // company-enum-value-upper-case
  value2
};

void qualsevol_patata() {} // company-global-variable-module-prefix

int main()
{
  int this_is_a_local_variable = 0;
  int this_in_NOT = 0; // company-local-variable-snake-case
  std::cout << "This is a very high line with more than 80 characters, which should trigger the line length check in checkpp." << std::endl;
  return 0;
}