#define MAXLD	15		// max live data fields

void init_windows (int argc, char *argv[]);
void live_data_update (struct can_frame *frame);
void *update_window (void *ptr);
void display_tune (int display);

