#ifndef OLED_DISPLAY_H_
#define OLED_DISPLAY_H_

#ifdef __cplusplus
extern "C"
{
#endif

void OLED_Display_Init(void);
void OLED_Display_RequestRefresh(void);
void OLED_Display_Task(void);
void OLED_Display_NextPage(void);
void OLED_Display_PrevPage(void);

#ifdef __cplusplus
}
#endif

#endif /* OLED_DISPLAY_H_ */
