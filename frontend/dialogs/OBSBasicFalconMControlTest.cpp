#include "OBSBasicFalconMControl.hpp"

#include <QDialog>
#include <QWidget>

#include <type_traits>

static_assert(std::is_base_of_v<QWidget, OBSBasicFalconMControl>);
static_assert(!std::is_base_of_v<QDialog, OBSBasicFalconMControl>);

int main()
{
	return 0;
}
