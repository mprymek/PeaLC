#include "locks.h"

EventGroupHandle_t global_event_group;

int locks_init()
{
	if ((global_event_group = xEventGroupCreate()) == NULL) {
		return -1;
	}

	return 0;
}
