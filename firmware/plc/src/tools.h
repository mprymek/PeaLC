#define MAX(a, b)                                                              \
	({                                                                     \
		__typeof__(a) _a = (a);                                        \
		__typeof__(b) _b = (b);                                        \
		_a > _b ? _a : _b;                                             \
	})

#define MIN(a, b)                                                              \
	({                                                                     \
		__typeof__(a) _a = (a);                                        \
		__typeof__(b) _b = (b);                                        \
		_a < _b ? _a : _b;                                             \
	})

/*
    Usage:

    MAX_ONCE_PER(UI_MIN_PLC_TICK_DELAY, {
        do_something();
    });
*/
#define MAX_ONCE_PER(period, block)                                            \
	if (period == 0) {                                                     \
		block                                                          \
	} else {                                                               \
		static uint32_t last_run = 0;                                  \
		uint32_t now = hal_uptime_msec();                              \
		if (now - last_run >= (period)) {                              \
			last_run = now;                                        \
			block                                                  \
		}                                                              \
	}

// Integer division with round up
#define DIV_UP(a, b) (((a) / (b)) + ((a % b == 0) ? 0 : 1))
