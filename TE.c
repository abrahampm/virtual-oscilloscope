#include "toolbox.h"
#include <analysis.h>
#include <ansi_c.h>
#include <string.h>
#include <DAQmxIOctrl.h>
#include <NIDAQmx.h>
#include <cvirte.h>		
#include <userint.h>
#include "TE.h"

#define DAQmxErrorCheck(status)\
	if(DAQmxFailed(status)) {\
		DAQmxGetExtendedErrorInfo(errBuff,2048);\
		STOP(panelHandle,PANEL_COMMANDBUTTONRUNSTOP,EVENT_COMMIT,NULL,0,0);\
		MessagePopup("DAQmx Error",errBuff);\
		return 0;\
	} else

int32 CVICALLBACK PlotData(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData); 

void CVICALLBACK DISABLEFILTER (int menuBar, int menuItem, void *callbackData,int panel);
int CVICALLBACK START (int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int CVICALLBACK STOP (int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void formatTime(double time,char* str);

static int panelHandle,panelHandle0,panelHandle1,panelHandle2,panelHandle3,menuBarHandle;
static TaskHandle Task1;
double xscale = 1.0, yscale = 1.0, trigger = 0.0, yoffset = 0.0;   //Scale and offset coefficients

int srate = 100000;    //Sampling frequency [samples/seconds]
int spc = 1000;        //Samples per channel (Sample's buffer size)
int maxpps = 10000;     //Maximum points per screen (Strip chart graph)
int minpps = 10;		//Minimum points per screen (Strip chart graph)
int pps = 10000;        //Points per screen (Strip chart graph)
int pp = 0;				//Points already plotted
int fftpoints = 8192;  //Minium number of points to perform FFT
int fftp = 0;           //Points already saved on fftbuffer
double refmin = -1, refmax = 1;//Analog input reference minium and maxium
double ymin = -1, ymax = 1;	//Y axis minium and maxium

BOOL oflag=FALSE;       //Graph cursor overflow flag for triggering
BOOL cflag = FALSE;     //Indicates when buffer[i-1] was less than buffer[i] for trigger threshold detector
BOOL panel1=FALSE,panel2=FALSE,panel2_1=FALSE;
BOOL filtering=FALSE,triggering=TRUE;

int MENUITEMID=0;

PFFTTable fft_table=NULL;
NIComplexNumber* fft=NULL;

int pcount=0;			 //Number of buffers to discard for triggering
												   
int forder=50, fc1=1000, fc2=10000; //Filter parameters
double *fcoef = NULL;				//Filter coeficients
double *magbuffer = NULL,*phabuffer = NULL;//FFT magnitude and phase buffer
double *convbuffer = NULL,*tconvbuffer=NULL;
float64	*sbuffer=NULL,*fftbuffer=NULL,*pbuffer=NULL;    //Sample's buffer

char        errBuff[2048]={'\0'};	

int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "TE.uir", PANEL)) < 0)
		return -1;
	if ((panelHandle0 = LoadPanel (0, "TE.uir", PANEL_1)) < 0)
		return -1;
	if ((panelHandle1 = LoadPanel (0, "TE.uir", PANEL_2)) < 0)
		return -1;
	if ((panelHandle2 = LoadPanel (0, "TE.uir", PANEL_3)) < 0)
		return -1;
	if ((panelHandle3 = LoadPanel (0, "TE.uir", PANEL_4)) < 0)
		return -1;
	if ((menuBarHandle = LoadMenuBar (panelHandle, "TE.uir", MENUBAR)) < 0)
		return -1;
	
	//Main stripchart settings
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_PLOT_BGCOLOR, VAL_BLACK);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_POINTS_PER_SCREEN, maxpps);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_SWEEP_LINE_COLOR, VAL_BLACK);
	SetCtrlVal (panelHandle, PANEL_SRATE, srate);
	char str[20]={'\0'};
	formatTime((double)pps/(10*srate), str);
	SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_XNAME, str);
	SetAxisScalingMode (panelHandle, PANEL_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, ymin, ymax);

	//Phase spectrum stripchart settings
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_PLOT_BGCOLOR, VAL_WHITE);
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_POINTS_PER_SCREEN, fftpoints);
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle0, PANEL_1_STRIPCHART1, ATTR_XUSE_LABEL_STRINGS, 1);
	for(float i=0,j=0,k=-1.0;i<=fftpoints;i+=(float)fftpoints/10,j++,k+=0.2) {
		sprintf(str, "%.00f", k*srate/2);
		InsertAxisItem (panelHandle0, PANEL_1_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
		InsertAxisItem (panelHandle1, PANEL_2_STRIPCHART2, VAL_BOTTOM_XAXIS, j, str, i);
	}
	
	//Amplitude spectrum stripchart settings
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_PLOT_BGCOLOR, VAL_WHITE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_POINTS_PER_SCREEN, fftpoints/2+1);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_XUSE_LABEL_STRINGS, 1);
	
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_PLOT_BGCOLOR, VAL_WHITE);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_POINTS_PER_SCREEN, fftpoints);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_XUSE_LABEL_STRINGS, 1);
	for(float i=0,j=0,k=0.0;i<=fftpoints/2+1;i+=(float)fftpoints/20,j++,k+=0.1) {
		sprintf(str, "%.00f", k*srate/2);
		InsertAxisItem (panelHandle1, PANEL_2_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
	}	
	
	
	//Signal filtering panel attributes
	//Filter magnitude response stripchart
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_PLOT_BGCOLOR, VAL_WHITE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_POINTS_PER_SCREEN, forder+1);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_XUSE_LABEL_STRINGS, 1);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_SWEEP_LINE_COLOR, MakeColor(150,150,150));
	//Filter phase response stripchart
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_EDGE_STYLE, VAL_FLAT_EDGE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_GRAPH_BGCOLOR, VAL_OFFWHITE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_PLOT_BGCOLOR, VAL_WHITE);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_GRID_COLOR , MakeColor(150,150,150));	
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_XLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_YLABEL_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_XYNAME_COLOR , MakeColor(50,50,50));
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_POINTS_PER_SCREEN, forder+1);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_LEGEND_VISIBLE, 0);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_XUSE_LABEL_STRINGS, 1);
	SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_SWEEP_LINE_COLOR, MakeColor(150,150,150));
	for(float i=0,j=0,k=-1.0;i<=forder;i+=(float)forder/10,j++,k+=0.2) {
		sprintf(str, "%.00f", k*srate/2);
		InsertAxisItem (panelHandle2, PANEL_3_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
		InsertAxisItem (panelHandle2, PANEL_3_STRIPCHART2, VAL_BOTTOM_XAXIS, j, str, i);
	}
	SetCtrlVal (panelHandle2, PANEL_3_NUMERICFORDER, forder);
	SetCtrlVal (panelHandle2, PANEL_3_NUMERICFC1, fc1);
	SetCtrlVal (panelHandle2, PANEL_3_NUMERICFC1, fc2);
	
	//Main screen buttons
	SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CALLBACK_FUNCTION_POINTER, START);
	SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MIN_VALUE, ymin);
	SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MAX_VALUE, ymax);
	SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MIN_VALUE, ymin);
	SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MAX_VALUE, ymax);
	
	//Create physical channel
	NIDAQmx_NewPhysChanAICtrl (panelHandle, PANEL_CHANNEL, 0);
	
	DisplayPanel (panelHandle);
	
	RunUserInterface ();
	
	DiscardPanel (panelHandle);
	return 0;
}

int CVICALLBACK XSCALE (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char str[20]={'\0'};
	switch (event)
	{
		case EVENT_VAL_CHANGED:
			GetCtrlVal (panelHandle, control, &xscale);
			
			pps=minpps*pow(10.0,xscale*3);
			pp=0;
			oflag=TRUE;
			SetCtrlAttribute(panelHandle, PANEL_STRIPCHART1,ATTR_POINTS_PER_SCREEN, pps);
			formatTime((double)pps/(10*srate), str);
			SetCtrlAttribute(panelHandle, PANEL_STRIPCHART1, ATTR_XNAME, str);
			break;
	}
	return 0;
}

int CVICALLBACK YSCALE (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_VAL_CHANGED:
			GetCtrlVal (panelHandle, control, &yscale);
			SetAxisScalingMode (panelHandle, PANEL_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, ymin*yscale-yoffset, ymax*yscale-yoffset);
			break;
	}
	return 0;
}

int CVICALLBACK YOFFSET (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_VAL_CHANGED:
			GetCtrlVal (panelHandle, control, &yoffset);
			SetAxisScalingMode (panelHandle, PANEL_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, ymin*yscale-yoffset, ymax*yscale-yoffset);
			SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_YAXIS_OFFSET, yoffset);
			break;
	}
	return 0;
}

int CVICALLBACK TRIGGER (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			case EVENT_VAL_CHANGED:
			GetCtrlVal (panelHandle, control, &trigger);
			break;
	}
	return 0;
}

int CVICALLBACK START (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char channel[256];
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_CHANNEL, channel);
			
			DAQmxErrorCheck(DAQmxCreateTask ("Voltage measurement", &Task1));
			DAQmxErrorCheck(DAQmxCreateAIVoltageChan (Task1, channel, "", DAQmx_Val_RSE, refmin, refmax, DAQmx_Val_Volts, NULL));
			DAQmxErrorCheck(DAQmxCfgSampClkTiming (Task1, NULL, srate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, srate));
			
			if((sbuffer=(float64*)malloc(spc*sizeof(float64)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			pbuffer=sbuffer;
			if((fft=(NIComplexNumber*)malloc(fftpoints*sizeof(NIComplexNumber)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			if((fftbuffer=(float64*)malloc(fftpoints*sizeof(float64)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			if((magbuffer=(double*)malloc(fftpoints*sizeof(double)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			if((phabuffer=(double*)malloc(fftpoints*sizeof(double)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			
			fft_table=CreateFFTTable(fftpoints);
			
			GetCtrlVal (panelHandle, PANEL_CHECKBOXTRIGGER, &triggering);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_DIMMED, !triggering);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_XSCALE, ATTR_DIMMED, 0);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YSCALE, ATTR_DIMMED, 0);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_DIMMED, 0);
			//SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_LEGEND_VISIBLE, 1);
			SetCtrlAttribute (panelHandle, PANEL_SRATE, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_CHECKBOXTRIGGER, ATTR_DIMMED, 0);
			SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_2, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_LABEL_TEXT, "__STOP");
			//SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CMD_BUTTON_COLOR, VAL_RED);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_SHORTCUT_KEY, VAL_MENUKEY_MODIFIER|'S');
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CALLBACK_FUNCTION_POINTER, STOP);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRESET, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_CHANNEL, ATTR_DIMMED, 1);
			
			DAQmxErrorCheck(DAQmxRegisterEveryNSamplesEvent (Task1, DAQmx_Val_Acquired_Into_Buffer, spc, 0, PlotData, NULL));
			DAQmxErrorCheck(DAQmxStartTask (Task1));
			
			break;
	}
	return 0;
}

int CVICALLBACK STOP (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if(Task1!=0){
				DAQmxStopTask(Task1);
				DAQmxClearTask(Task1);
				Task1 = 0;
			}
			if(sbuffer) {
				free(sbuffer);
				sbuffer = NULL;
			}
			if(fftbuffer) {
				free(fftbuffer);
				fftbuffer = NULL;
			}
			if(fft) {
				free(fft);
				fft = NULL;
			}
			if(fft_table) {
				DestroyFFTTable(fft_table);
				fft_table=NULL;
			}
			if(magbuffer) {
				free(magbuffer);
				magbuffer = NULL;
			}
			if(phabuffer) {
				free(phabuffer);
				phabuffer = NULL;
			}
			
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_XSCALE, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YSCALE, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_DIMMED, 1);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_DIMMED, 1);
			//SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_LEGEND_VISIBLE, 0);
			SetCtrlAttribute (panelHandle, PANEL_SRATE, ATTR_DIMMED, 0);
			SetCtrlAttribute (panelHandle, PANEL_CHECKBOXTRIGGER, ATTR_DIMMED, 1);
			SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_2, ATTR_DIMMED, 0);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_LABEL_TEXT, "__RUN");
			//SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CMD_BUTTON_COLOR, VAL_GREEN);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_SHORTCUT_KEY, VAL_MENUKEY_MODIFIER|'R');
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CALLBACK_FUNCTION_POINTER, START);
			SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRESET, ATTR_DIMMED, 0);
			SetCtrlAttribute (panelHandle, PANEL_CHANNEL, ATTR_DIMMED, 0);
			
			break;
	}
	return 0;
}

int32 CVICALLBACK PlotData(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData){
	int32 nsread=0,npoints=0;
	//Triggering
	int sindex=0,count=0;
	
	DAQmxReadAnalogF64 (taskHandle, spc, 10.0, DAQmx_Val_GroupByChannel, sbuffer, spc, &nsread, NULL);

	if(nsread>0){
		
		//Overlap and add convolution
		if(filtering){ 
			ConvolveEx (sbuffer, nsread, fcoef, forder, ALGORITHM_CONCOR_DIRECT, convbuffer);
			for(int i=nsread,j=0;i<nsread+forder-1;i++,j++){
				convbuffer[j]+=tconvbuffer[j];
				tconvbuffer[j]=convbuffer[i];
			}
		}
		if(oflag==FALSE || pcount>5){
			if(pp+nsread>=pps) { //Graph cursor (pp)
				npoints = pps - 1 - pp;
				pp=0;pcount=0;
			} else {
				npoints = nsread;
				pp+=nsread;
			}
			if(npoints>0)
				PlotStripChart (panelHandle, PANEL_STRIPCHART1, pbuffer, npoints, sindex, 0, VAL_DOUBLE); 
		} else {
			pcount++;
		}
		
		if(pp==0) oflag=TRUE; 
		
		for(int i=0;i< nsread;i++){ 
			if((panel1 || panel2 || panel2_1) && fftp<fftpoints) //Fill fftbuffer to perform FFT
				fftbuffer[fftp++]=pbuffer[i];
			
			if(triggering && oflag==TRUE && i>=npoints){    //Triggering
				if(pbuffer[i]<trigger) cflag=TRUE;
				else if(cflag==TRUE){
					if(++count>10){
						npoints=nsread-(i+1-10);
						PlotStripChart (panelHandle, PANEL_STRIPCHART1, pbuffer, npoints, i-10, 0, VAL_DOUBLE);
						pp+=npoints;
						oflag=FALSE;cflag=FALSE;
					}
				}
			}
		}
		if(npoints>0)
			
		
		
		//FFT
		if(fftp == fftpoints && (panel1 || panel2 || panel2_1)){
	        FFTEx (fftbuffer, fftp, fftp, fft_table, 1, fft);       //Perform FFT
			for(int i=0;i<fftp;i++)
				ToPolar((fft+i)->real,(fft+i)->imaginary,magbuffer+i,phabuffer+i);
			
			//Phase spectrum
			if(panel1){
				ClearStripChart (panelHandle0, PANEL_1_STRIPCHART1);
				PlotStripChart (panelHandle0, PANEL_1_STRIPCHART1, phabuffer, fftp, 0, 0, VAL_DOUBLE);
			}
			//Single sided amplitude spectrum
			if(panel2){		   
				ClearStripChart (panelHandle1, PANEL_2_STRIPCHART1);
				PlotStripChart (panelHandle1, PANEL_2_STRIPCHART1,magbuffer, fftp/2, fftp/2, 0, VAL_DOUBLE);
			} else if(panel2_1){ //Amplitude spectrum
				ClearStripChart (panelHandle1, PANEL_2_STRIPCHART2);
				PlotStripChart (panelHandle1, PANEL_2_STRIPCHART2,magbuffer, fftp, 0, 0, VAL_DOUBLE);
			}
			fftp = 0;
		}
    }
	return 0;
}


int CVICALLBACK SETSRATE (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char str[25]={'\0'};
	int tsrate=0;
	switch (event)
	{
		case EVENT_VAL_CHANGED:
			GetCtrlVal(panel,control,&tsrate);	
			if(tsrate<=100000 && tsrate>0){
				srate=tsrate;
				formatTime((double)pps/(10*srate), str);
				SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_XNAME, str);
				for(float i=0,j=0,k=0.0;i<=fftpoints/2+1;i+=(float)fftpoints/20,j++,k+=0.1) {
					sprintf(str, "%.00f", k*srate/2);
					ReplaceAxisItem (panelHandle1, PANEL_2_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
				}
				for(float i=0,j=0,k=-1.0;i<=fftpoints;i+=(float)fftpoints/10,j++,k+=0.2) {
					sprintf(str, "%.00f", k*srate/2);
					ReplaceAxisItem (panelHandle0, PANEL_1_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
					ReplaceAxisItem (panelHandle1, PANEL_2_STRIPCHART2, VAL_BOTTOM_XAXIS, j, str, i);
				}
				for(float i=0,j=0,k=-1.0;i<=forder;i+=(float)forder/10,j++,k+=0.2) {
					sprintf(str, "%.00f", k*srate/2);
					ReplaceAxisItem (panelHandle2, PANEL_3_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
					ReplaceAxisItem (panelHandle2, PANEL_3_STRIPCHART2, VAL_BOTTOM_XAXIS, j, str, i);
				}
				
				if((float)spc/srate<0.0025)
					spc=(int)0.0025*srate;
				else if(spc>=srate)
					spc=srate/100;
			} else {
				MessagePopup("Error","Sampling rate out of range.\n\
				The following condition must be met 0<rate<=100000.");
				return -1;
			}
			break;
	}
	return 0;
}

void formatTime(double timeSec, char* str){
	double ms = timeSec*1000;
	double us = timeSec*1000000;
	sprintf(str, ms > 1 ? "t(%.3lf  ms/div)" : "t(%.3lf  us/div)", ms > 1 ? ms : us);
	return;
}

void CVICALLBACK SHOWPANEL1 (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	DisplayPanel (panelHandle0);
	panel1=TRUE;
}

void CVICALLBACK SHOWPANEL2 (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	DisplayPanel (panelHandle1);
	switch(menuItem){
		case MENUBAR_MENU2_ITEM2_2://Single sided amplitude spectrum option
			SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_VISIBLE, 0);
			SetPanelAttribute(panelHandle1, ATTR_TITLE, "Single sided amplitude spectrum");
			panel2=TRUE;
			panel2_1=FALSE;
			break;
		case MENUBAR_MENU2_ITEM1_2://Amplitude spectrum option
			SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_VISIBLE, 0);
			SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_VISIBLE, 1);
			SetPanelAttribute(panelHandle1, ATTR_TITLE, "Amplitude spectrum");
			panel2_1=TRUE;
			panel2=FALSE;
			break;
	}
	
	
}

void CVICALLBACK SHOWPANEL3 (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	InstallPopup (panelHandle2);
	switch(menuItem){
		case MENUBAR_MENU1_ITEM2://Add lowpass filter option
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 0);
			break;
		case MENUBAR_MENU1_ITEM3://Add highpass filter option
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 0);
			SetCtrlVal (panelHandle2, PANEL_3_NUMERICFORDER, 51);
			break;
		case MENUBAR_MENU1_ITEM1://Add bandpass filter option
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 1);
			break;
	}
	DFILTER (panel, menuBar, EVENT_COMMIT, callbackData, 0, 0);
	return;
}

void CVICALLBACK SHOWSETTINGS (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	InstallPopup(panelHandle3);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICSPC, spc);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICFFTP, fftpoints);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICYAXISMIN, ymin);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICYAXISMAX, ymax);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICREFMIN, refmin);
	SetCtrlVal (panelHandle3, PANEL_4_NUMERICREFMAX, refmax);
}

int CVICALLBACK PANELCALLBACK (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	char str[7]={'\0'};
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			GetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_LABEL_TEXT, str);
			if(strcmp (str, "__RUN")==0){
				if(convbuffer)
					free(convbuffer);
				if(tconvbuffer)
					free(tconvbuffer);
				if(fcoef)
					free(fcoef);
				QuitUserInterface(0);
			}
			else
				MessagePopup("Atención","Detenga la tarea en ejecución antes de cerrar la aplicación.");
			break;
	}
	return 0;
}

int CVICALLBACK PANEL1CALLBACK (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			HidePanel(panelHandle0);
			panel1=FALSE;
			break;
	}
	return 0;
}

int CVICALLBACK PANEL2CALLBACK (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			HidePanel (panelHandle1);
			panel2=FALSE;
			panel2_1=FALSE;
			break;
	}
	return 0;
}

int CVICALLBACK PANEL3CALLBACK (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			RemovePopup(panelHandle2);
			break;
	}
	return 0;
}

int CVICALLBACK PANEL4CALLBACK (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			RemovePopup(panelHandle3);
			break;
	}
	return 0;
}

int CVICALLBACK SETLPF (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 0);
			break;
	}
	return 0;
}

int CVICALLBACK SETBPF (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 1);
			break;
	}
	return 0;
}

int CVICALLBACK SETHPF (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONLPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONHPF, ATTR_CTRL_VAL, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_RADIOBUTTONBPF, ATTR_CTRL_VAL, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_NUMERICFC2, ATTR_VISIBLE, 0);
			break;
	}
	return 0;
}

int CVICALLBACK DFILTER (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{   int s[3]={0,0,0}, wtype=4;
	NIComplexNumber *fft1 = NULL; //Buffer for filter impulse response fft result
	double *mbuffer,*pbuffer;     //Magnitude and phase buffer for fft result conversion
	char str[20]={'\0'};
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle2, PANEL_3_RADIOBUTTONLPF, &s[0]);
			GetCtrlVal (panelHandle2, PANEL_3_RADIOBUTTONHPF, &s[1]);
			GetCtrlVal (panelHandle2, PANEL_3_RADIOBUTTONBPF, &s[2]);
			GetCtrlVal (panelHandle2, PANEL_3_NUMERICFC1, &fc1);
			GetCtrlVal (panelHandle2, PANEL_3_NUMERICFC2, &fc2);
			GetCtrlVal (panelHandle2, PANEL_3_NUMERICFORDER, &forder);
			GetCtrlVal (panelHandle2, PANEL_3_RINGWTYPE, &wtype);
			if(spc<forder){ 
				MessagePopup("Error","Buffer size too small to perform filtering.\n\
							  Stop the task and increase the buffer size.\n\
							  The following conditions must be met:  bsize >= forder");
				return -1;
			}
			if(forder<=2 || forder>100){
				MessagePopup("Error","The following conditions must be met:  2 < forder <= 100");
				return -1;
			}
			if(fc1<=0 || fc1>srate/2){
				MessagePopup("Error","The following conditions must be met:  0 < fc <= fs/2");
				return -1;
			}
			if(fcoef){
				free(fcoef);
				fcoef=NULL;
			}
			if((fcoef=(double*)malloc(forder*sizeof(double)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			
			if(s[0]){
				Wind_LPF(srate,fc1,forder,fcoef,wtype);
			} else	
			if(s[1]){
				if(forder%2==0){
					MessagePopup("Error","Filter order must be odd for this filter");
					return -1;
				}
				Wind_HPF(srate,fc1,forder,fcoef,wtype);
			} else 
			if(s[2]){
				if(!(fc1<fc2 && fc2<srate/2 && fc1>0)){
					MessagePopup("Error","The following conditions must be met:  0 < f_low <= f_high <= fs/2");
					return -1;
				}
				Wind_BPF(srate,fc1,fc2,forder,fcoef,wtype);
			}
			
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_POINTS_PER_SCREEN, forder%2==0?forder+1:forder);
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_POINTS_PER_SCREEN, forder%2==0?forder+1:forder);
			for(float i=0,j=0,k=-1.0;i<forder;i+=(float)(forder%2==0?forder:forder-1)/10,j++,k+=0.2) {
				sprintf(str, "%.00f", k*srate/2);
				ReplaceAxisItem (panelHandle2, PANEL_3_STRIPCHART1, VAL_BOTTOM_XAXIS, j, str, i);
				ReplaceAxisItem (panelHandle2, PANEL_3_STRIPCHART2, VAL_BOTTOM_XAXIS, j, str, i);
			}
			
			//Memory allocation to perform FFT
			if((fft1=(NIComplexNumber*)malloc(forder*sizeof(NIComplexNumber)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			if((mbuffer=(double*)malloc(forder*sizeof(double)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			if((pbuffer=(double*)malloc(forder*sizeof(double)))==NULL){
				MessagePopup("Error","Not enough memory");
				return -1;
			}
			
			FFTEx(fcoef,forder,forder,NULL,1,fft1);
			for(int i=0;i<forder;i++)
				ToPolar((fft1+i)->real,(fft1+i)->imaginary,mbuffer+i,pbuffer+i);

			
			PlotStripChart (panelHandle2, PANEL_3_STRIPCHART1, mbuffer, forder, 0, 0, VAL_DOUBLE);
			PlotStripChart (panelHandle2, PANEL_3_STRIPCHART2, pbuffer, forder, 0, 0, VAL_DOUBLE);
			
			free(mbuffer);
			free(pbuffer);
			free(fft1);
			break;
	}
	return 0;
}

int CVICALLBACK APPLYFILTER (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if(DFILTER (panel, control, event, callbackData, eventData1, eventData2)==0){
				
				if((convbuffer=(double*)malloc((forder+spc-1)*sizeof(double)))==NULL){
					MessagePopup("Error","Not enough memory");
					return -1;
				}
				if((tconvbuffer=(double*)malloc((forder-1)*sizeof(double)))==NULL){
					MessagePopup("Error","Not enough memory");
					return -1;
				}
				for(int i=0;i<forder-1;i++)
					tconvbuffer[i]=0;
				pbuffer=convbuffer;
				filtering=TRUE;
				RemovePopup(panelHandle2);
				MENUITEMID = NewMenuItem (menuBarHandle, MENUBAR_MENU1, "Disable filter", -1, 0, DISABLEFILTER, 0);
				SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM2, ATTR_DIMMED, 1);
				SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM3, ATTR_DIMMED, 1);
				SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM1, ATTR_DIMMED, 1);
			}
			break;
	}
	return 0;
}

void CVICALLBACK DISABLEFILTER (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	pbuffer=sbuffer;
	if(fcoef){
		free(fcoef);
		fcoef=NULL;
	}
	if(convbuffer){
		free(convbuffer);
		convbuffer=NULL;
	}
	if(tconvbuffer){
		free(tconvbuffer);
		tconvbuffer=NULL;
	}
	forder=10;
	fc1=500;
	fc2=1000;
	filtering=FALSE;
	SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM2, ATTR_DIMMED, 0);
	SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM3, ATTR_DIMMED, 0);
	SetMenuBarAttribute (menuBarHandle, MENUBAR_MENU1_ITEM1, ATTR_DIMMED, 0);
	DiscardMenuItem (menuBarHandle, MENUITEMID);
	
}

int CVICALLBACK SHOWMAGN (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_VISIBLE, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_TAB0, ATTR_LABEL_BOLD, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_TAB1, ATTR_LABEL_BOLD, 0);
			break;
	}
	return 0;
}

int CVICALLBACK SHOWPHAS (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART1, ATTR_VISIBLE, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_STRIPCHART2, ATTR_VISIBLE, 1);
			SetCtrlAttribute (panelHandle2, PANEL_3_TAB0, ATTR_LABEL_BOLD, 0);
			SetCtrlAttribute (panelHandle2, PANEL_3_TAB1, ATTR_LABEL_BOLD, 1);
			break;
	}
	return 0;
}

int CVICALLBACK TOGGLETRIGGER (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,control,&triggering);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_DIMMED, !triggering);
			break;
	}
	return 0;
}

int CVICALLBACK APPLYSETTINGS (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int tspc=0,tfftp=0;
	double tymin=0,tymax=0,trefmin=0,trefmax=0;
	switch (event)
	{
		case EVENT_COMMIT:
			
			//Validating buffer size value
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICSPC, &tspc);
			if((float)tspc/srate<0.0025 || tspc>srate){
				MessagePopup("Error",
					"Invalid buffer size.\n\
					 The application needs at least 2.5ms to process data before the buffer gets full.\n\
					 Using this buffer size at the current sampling rate would lead to a runtime error.\n\
					 The following condition must be met bsize/srate>0.0025");
				return -1;
			}
			else {
				spc=tspc;
			}
			
			//Validating ADC reference values 
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICREFMIN, &trefmin);
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICREFMAX, &trefmax);
			if(trefmin<-10 || trefmax>10 || trefmin > trefmax || trefmin==trefmax){
				MessagePopup("Error",
					"Invalid ADC reference range.\n\
					 The following condition must be met -10<=refmin<refmax<=10");
				return -1;		
			} else {
				refmin=trefmin;
				refmax=trefmax;
			}
			
			//Validating y axis values 
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICYAXISMIN, &tymin);
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICYAXISMAX, &tymax);
			if(tymin<-10 || tymax>10 || tymin > tymax || tymin==tymax){
				MessagePopup("Error",
					"Invalid y axis range.\n\
					 The following condition must be met -10<=ymin<ymax<=10");
				return -1;		
			} else {
				ymin=tymin;
				ymax=tymax;
				SetAxisScalingMode (panelHandle, PANEL_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, ymin, ymax);
				SetCtrlAttribute (panelHandle, PANEL_COMMANDBUTTONRUNSTOP, ATTR_CALLBACK_FUNCTION_POINTER, START);
				SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MIN_VALUE, ymin);
				SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MAX_VALUE, ymax);
				SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MIN_VALUE, ymin);
				SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MAX_VALUE, ymax);
			}
			
			//Validating FFT points
			GetCtrlVal (panelHandle3, PANEL_4_NUMERICFFTP, &tfftp);
			BOOL flag=FALSE;
			for(int i=2;i<=8192;i*=2)
				if(tfftp%i==0) {flag=TRUE;break;}
			if(tfftp<3 || tfftp > 10000 || !flag){
				MessagePopup("Error",
					"Invalid FFT points value.\n\
					 FFT points must be power of 2.\n\
					 The following condition must be met 3<fft<=10000");
				return -1;	
			} else {
				fftpoints=tfftp;
				SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, 0, tfftp);
				SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART1, ATTR_YAXIS_GAIN, (double)1/tfftp);
				SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART2, VAL_LEFT_YAXIS, VAL_MANUAL, 0, tfftp);
				SetCtrlAttribute (panelHandle1, PANEL_2_STRIPCHART2, ATTR_YAXIS_GAIN, (double)1/tfftp);
			}
			
			RemovePopup(panelHandle3);
			break;
	}
	
	return 0;
}

int CVICALLBACK YAUTOSCALE (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	BOOL d=FALSE;
	switch (event)
	{
		case EVENT_COMMIT:
		GetCtrlVal (panelHandle1, PANEL_2_CHECKBOXAUTOSCALE, &d);
		if(d){
			SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART1, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, fftpoints);
			SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART2, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, fftpoints);
		} else {
			SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, 0, fftpoints);
			SetAxisScalingMode (panelHandle1, PANEL_2_STRIPCHART2, VAL_LEFT_YAXIS, VAL_MANUAL, 0, fftpoints);
		}
		break;
	}
	return 0;
}

int CVICALLBACK RESET (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			ClearStripChart (panelHandle, PANEL_STRIPCHART1);
			ClearStripChart (panelHandle0, PANEL_1_STRIPCHART1);
			ClearStripChart (panelHandle1, PANEL_2_STRIPCHART1);
			ClearStripChart (panelHandle1, PANEL_2_STRIPCHART2);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MIN_VALUE, ymin);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_MAX_VALUE, ymax);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MIN_VALUE, ymin);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_TRIGGER, ATTR_MAX_VALUE, ymax);
			SetAxisScalingMode (panelHandle, PANEL_STRIPCHART1, VAL_LEFT_YAXIS, VAL_MANUAL, ymin, ymax);
			SetCtrlAttribute (panelHandle, PANEL_STRIPCHART1, ATTR_YAXIS_OFFSET, 0.0);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YSCALE, ATTR_CTRL_VAL, 1.0);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_YOFFSET, ATTR_CTRL_VAL, 0.0);
			SetCtrlAttribute (panelHandle, PANEL_NUMERICKNOB_XSCALE, ATTR_CTRL_VAL, 1.0);
			SetCtrlAttribute(panelHandle, PANEL_STRIPCHART1,ATTR_POINTS_PER_SCREEN, maxpps);
			pp=0;
			break;
	}
	return 0;
}
