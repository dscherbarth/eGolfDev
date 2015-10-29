//
// scope.h
//
#define VIEWSNAP	1
#define VIEWTREND	2

void create_scope (GtkWidget *fixed);
void update_scope_snapshot_data (int16_t *v1, int16_t *v2, int16_t *v3, int16_t *v4, int16_t *v5, int16_t *v6, int16_t *v7, int count);
void update_scope_snapshot_view (int channel, int visible);
void update_scope_trend_data (int v1, int v2, int v3, int v4);
void update_scope_trend_view (int channel, int visible);
void display_scope (int display);
