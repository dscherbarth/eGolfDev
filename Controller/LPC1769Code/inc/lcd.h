// prototypes for the lcd interface

#define DATA	0x40
#define MSG		0x41

void lcd_clear(void);
void lcd_cursor (int row, int col);
void lcd_write(char *buffer, int len);
void lcd_init(void);
void data_write(char type, char *buffer, int len);
void data_request(char *buffer, uint8_t len);
void lcd_cursor_on ( uint32_t R, uint32_t C );
void lcd_cursor_off ( void );
void lcd_init_cmd(void);
