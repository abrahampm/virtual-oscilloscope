/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2018. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: PANELCALLBACK */
#define  PANEL_NUMERICKNOB_XSCALE         2       /* callback function: XSCALE */
#define  PANEL_NUMERICKNOB_YSCALE         3       /* callback function: YSCALE */
#define  PANEL_NUMERICKNOB_TRIGGER        4       /* callback function: TRIGGER */
#define  PANEL_NUMERICKNOB_YOFFSET        5       /* callback function: YOFFSET */
#define  PANEL_CHANNEL                    6
#define  PANEL_STRIPCHART1                7
#define  PANEL_SRATE                      8       /* callback function: SETSRATE */
#define  PANEL_CHECKBOXTRIGGER            9       /* callback function: TOGGLETRIGGER */
#define  PANEL_COMMANDBUTTONRUNSTOP       10
#define  PANEL_COMMANDBUTTONRESET         11      /* callback function: RESET */

#define  PANEL_1                          2       /* callback function: PANEL1CALLBACK */
#define  PANEL_1_STRIPCHART1              2

#define  PANEL_2                          3       /* callback function: PANEL2CALLBACK */
#define  PANEL_2_STRIPCHART2              2
#define  PANEL_2_STRIPCHART1              3
#define  PANEL_2_CHECKBOXAUTOSCALE        4       /* callback function: YAUTOSCALE */

#define  PANEL_3                          4       /* callback function: PANEL3CALLBACK */
#define  PANEL_3_STRIPCHART1              2
#define  PANEL_3_STRIPCHART2              3
#define  PANEL_3_RADIOBUTTONLPF           4       /* callback function: SETLPF */
#define  PANEL_3_RADIOBUTTONHPF           5       /* callback function: SETHPF */
#define  PANEL_3_RADIOBUTTONBPF           6       /* callback function: SETBPF */
#define  PANEL_3_TEXTMSG1                 7
#define  PANEL_3_TEXTMSG2                 8
#define  PANEL_3_TEXTMSG                  9
#define  PANEL_3_NUMERICFORDER            10
#define  PANEL_3_NUMERICFC2               11
#define  PANEL_3_NUMERICFC1               12
#define  PANEL_3_COMMANDBUTTON_2          13      /* callback function: APPLYFILTER */
#define  PANEL_3_COMMANDBUTTON            14      /* callback function: DFILTER */
#define  PANEL_3_RINGWTYPE                15
#define  PANEL_3_TAB1                     16      /* callback function: SHOWPHAS */
#define  PANEL_3_TAB0                     17      /* callback function: SHOWMAGN */

#define  PANEL_4                          5       /* callback function: PANEL4CALLBACK */
#define  PANEL_4_NUMERICREFMAX            2
#define  PANEL_4_NUMERICREFMIN            3
#define  PANEL_4_NUMERICYAXISMAX          4
#define  PANEL_4_NUMERICYAXISMIN          5
#define  PANEL_4_NUMERICFFTP              6
#define  PANEL_4_NUMERICSPC               7
#define  PANEL_4_COMMANDBUTTONSETT        8       /* callback function: APPLYSETTINGS */


     /* Menu Bars, Menus, and Menu Items: */

#define  MENUBAR                          1
#define  MENUBAR_MENU1                    2
#define  MENUBAR_MENU1_ITEM2              3       /* callback function: SHOWPANEL3 */
#define  MENUBAR_MENU1_ITEM3              4       /* callback function: SHOWPANEL3 */
#define  MENUBAR_MENU1_ITEM1              5       /* callback function: SHOWPANEL3 */
#define  MENUBAR_MENU2                    6
#define  MENUBAR_MENU2_ITEM2_2            7       /* callback function: SHOWPANEL2 */
#define  MENUBAR_MENU2_ITEM1_2            8       /* callback function: SHOWPANEL2 */
#define  MENUBAR_MENU2_ITEM3_2            9       /* callback function: SHOWPANEL1 */
#define  MENUBAR_MENU1_2                  10      /* callback function: SHOWSETTINGS */


     /* Callback Prototypes: */

int  CVICALLBACK APPLYFILTER(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK APPLYSETTINGS(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK DFILTER(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PANEL1CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PANEL2CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PANEL3CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PANEL4CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PANELCALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RESET(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SETBPF(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SETHPF(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SETLPF(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SETSRATE(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SHOWMAGN(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK SHOWPANEL1(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SHOWPANEL2(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SHOWPANEL3(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK SHOWPHAS(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK SHOWSETTINGS(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK TOGGLETRIGGER(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TRIGGER(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK XSCALE(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK YAUTOSCALE(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK YOFFSET(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK YSCALE(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
