#if !defined(ARDUINO)
#define max(a, b)                                                              \
	({                                                                     \
		__typeof__(a) _a = (a);                                        \
		__typeof__(b) _b = (b);                                        \
		_a > _b ? _a : _b;                                             \
	})

#define min(a, b)                                                              \
	({                                                                     \
		__typeof__(a) _a = (a);                                        \
		__typeof__(b) _b = (b);                                        \
		_a < _b ? _a : _b;                                             \
	})
#endif // #if !defined(ARDUINO)

/*
    Usage:

    MAX_ONCE_PER(UI_MIN_PLC_TICK_DELAY, {
        do_something();
    });
*/
#define MAX_ONCE_PER(period, block)                                            \
	{                                                                      \
		static uint32_t last_run = 0;                                  \
		uint32_t now = hal_uptime_msec();                              \
		if (now - last_run >= (period)) {                              \
			last_run = now;                                        \
			block                                                  \
		}                                                              \
	}
