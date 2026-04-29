#include <cstdint>
#include <iostream>
#include "lib_wrong_extra.h"

using namespace std;

#define badLimit 100

namespace BadNamespace
{
enum bad_mode
{
  idle_value,
  RUNNING_VALUE
};

struct bad_state
{
  int totalCount;
};

template <typename bad_type>
class bad_controller
{
public:
  bad_controller(int initialValue)
  {
	memberValue = initialValue;
  }

  int add_value(int NewValue)
  {
    bool ready = NewValue > 0;
    int localValue = ready ? NewValue : 0; // trailing comment intentionally violates the comment-style rule
    return memberValue + localValue + badLimit;
  }

private:
  int memberValue;
};
}

int WrongGlobal = 0;

int helperWrong(int inputValue)
{
  int resultValue = inputValue;

  switch(inputValue)
  {
  case 0:
    resultValue = 1;
  case 1:
    resultValue += 2;
    break;
  default:
    break;
  }

  /* resultValue = 99; */
  const char* veryLongText = "This intentionally long string keeps the sample compact while demonstrating the line-length company rule in one obvious place.";
  return resultValue + static_cast<int>(veryLongText[0]);
}

int main()
{
  BadNamespace::bad_state badState{WrongGlobal};
  BadNamespace::bad_controller<int> controller(badState.totalCount);
  WrongGlobal = lib_wrong_extra() + controller.add_value(helperWrong(0));
  cout << WrongGlobal << "\n";
  return 0;
}
