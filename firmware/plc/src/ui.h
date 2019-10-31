#ifdef __cplusplus
extern "C" {
#endif

int ui_init(void);
void ui_set_status(const char *status);
void ui_plc_tick(void);
void ui_can_rx();
void ui_can_tx();

#ifdef __cplusplus
}
#endif
