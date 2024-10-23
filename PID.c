#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pigpio.h>

// 목표 위치 제어용 변수
#define CLOCK 18
#define Pin1 23
#define Pin2 24
#define Pin3 25

// Encoder 제어용 변수(임시)
#define ENCODERA 17
#define ENCODERB 27
#define ENC2REDGEAR 216

// 모터 값 제어 변수(임시)
#define MOTOR1 19
#define MOTOR2 26
#define PGAIN 100
#define IGAIN 10
#define DGAIN 1
#define T 5000 // 마이크로 초 단위

// 그래프 저장용 배열 변수
#define NUM_ROWS 9000 
#define NUM_COLUMNS 2
#define DAQ_TIME 50000

int get_pos[3] = {};
int store_pos[5] = {};
int i = 0;
int j = 0;
int k = 0;

int encA, encB;
int encoderPosition = 0;
float redGearPosition = 0;

int targetpos = 0;
float errorPosition = 0;
float errorPositionBefore = 0;
float deltaError = 0;
float Pout = 0;
float Iout = 0;
float Dout = 0;
float PIDout = 0;
float Integral = 0;
float scaledPIDout = 0;
int scaledPIDoutint = 0;
unsigned int checkTime, checkTimeBefore, deltaTime, startTime;

int dataIndex = 0;
float dataArray[NUM_ROWS][NUM_COLUMNS];
char filename[100];
FILE* file;

void saveGraph() // 그래프 저장하는 함수
{
    FILE *gnuplot = popen("gnuplot -persistnet", "w");  
    fprintf(gnuplot, "set datafile separator ','\n"); 
    fprintf(gnuplot, "set terminal pngcairo size 800,600\n"); 
    fprintf(gnuplot, "set output '%s.png'\n", filename);  
    fprintf(gnuplot, "set title '%s Graph'\n", filename);
    fprintf(gnuplot, "set yrange [0:%lf]\n", targetpos*1.2);
    fprintf(gnuplot, "set xlabel 'Time [s]'\n");            
    fprintf(gnuplot, "set ylabel 'Position [rev]'\n");      
    fprintf(gnuplot, "plot '%s' using 1:2 with linespoints title '%s', %f with lines lt 2 title 'Ref'\n", filename_csv, filename, targetpos);
    fprintf(gnuplot, "set output\n");
    pclose(gnuplot);
}

void updateDataArray() // 시간에 따른 모터 위치 저장하는 배열 함수
{
    dataArray[dataIndex][0] = (float)(checkTime - startTime)/1000000.0;
    dataArray[dataIndex][1] = redGearPosition;

    if (dataIndex == NUM_ROWS){ // 데이터 배열 NUM_ROWS 초과시 그래프 저장 및 종료

        printf("\nEXIT PROGRAM\n");
        gpioPWM(MOTOR1, 0);
        gpioPWM(MOTOR2, 0);

        for (int p = 0; p < dataIndex; p++)
        {
        fprintf(file, "%.3f,%.3f\n", dataArray[p][0], dataArray[p][1]); 
        }
        fclose(file);

        saveGraph();
        exit(0);
    }
    else dataIndex++;
}

void handle_sigint(int sig) // 컨트롤 + c 사용시 배열 저장 및 그래프 출력
{
    printf("\nEXIT PROGRAM\n");
    gpioPWM(MOTOR1, 0);
    gpioPWM(MOTOR2, 0);

    for (int p = 0; p < dataIndex; p++)
    {
        fprintf(file, "%.3f,%.3f\n", dataArray[p][0], dataArray[p][1]); 
    }
    fclose(file);

    saveGraph();    
    exit(0);
}

void SubscribeClock(void) // clock이 rising edge일때마다 호출되는 함수
{
    int pin1 = gpioRead(Pin1);
    int pin2 = gpioRead(Pin2);
    int pin3 = gpioRead(Pin3);

    if (i == 3){
        get_pos[0] = get_pos[1];
        get_pos[1] = get_pos[2];
        get_pos[2] = pin1*1 + pin2*2 + pin3*4;

        if (get_pos[0] == get_pos[1] && get_pos[1] == get_pos[2]){ // 3개 원소가 모두 같을 때
            if (get_pos[0] != 0){
                store_pos[j] = -get_pos[0];
                printf("%d stored!\n", store_pos[j]);
                j++;
                i = 0; get_pos[1] = 0; get_pos[2] = 0;// 중복 탐지를 피하기 위해 배열 초기화
            }
        }
        else if (get_pos[0] == get_pos[1]){ // 앞 2개의 원소가 같을 때
            if (get_pos[0] != 0){
                store_pos[j] = get_pos[0];
                printf("%d stored!\n", store_pos[j]);
                j++;
            }
        }
        else if (get_pos[1] == 0 && get_pos[2] == 0){ // 뒷 2개의 원소가 0일 때(target position 실행 시작)
            PublishTargetPos(&targetpos, store_pos[k]); // target position 반환함수 실행
            k++;
        }
    }
    else{
        get_pos[i] = pin1*1 + pin2*2 + pin3*4;
        i++;
        switch (i)
        {
        case 2:
            if (get_pos[0] == get_pos[1] && j > 0) // store_pos에 저장된 target position이 있으면서 첫 2개 원소가 같을 때
            {   
                if (get_pos[0] == 0){ // 만약 첫 2개의 원소값이 모두 0이라면
                    PublishTargetPos(&targetpos, store_pos[k]); // target position 반환함수 실행
                    k++;
                }
            }
            break;
        case 3:
            if (get_pos[0] == get_pos[1] && get_pos[1] == get_pos[2]){ // 3개 원소가 모두 같을 때
                store_pos[j] = -get_pos[0];
                printf("%d stored!\n", store_pos[j]);
                j++;
                i = 0; get_pos[1] = 0; get_pos[2] = 0;// 중복 탐지를 피하기 위해 배열 초기화
            }
            else if (get_pos[0] == get_pos[1]){ // 첫 2개 원소만 같을 때
                if (get_pos[0] != 0){ // 만약 첫 2개의 원소값이 모두 0이라면
                    store_pos[j] = get_pos[0];
                    printf("%d stored!\n", store_pos[j]);
                    j++;
                }
            }
            break;
        default:
            break;
        }
    }
}

void PublishTargetPos(int *targetpos, int new_targetpos) // targetpos를 반환하기 위한 함수 정의. targetpos를 포인터로 수정해서 main 함수에서 사용가능하게 바꿨습니다.
{
    *targetpos = new_targetpos;
    printf("target position : %d\n",*targetpos);
    
}

void funcEncoderA() // Encoder A에서 위치 받는 함수 (수업시간 참조)
{
    encA = gpioRead(ENCODERA);
    encB = gpioRead(ENCODERB);

    if (encA == 1) {
        if (encB == 0) encoderPosition++;
        else encoderPosition--;
    }

    else {
        if (encB == 0) encoderPosition--;
        else encoderPosition++;
    }

    redGearPosition = (float)encoderPosition / ENC2REDGEAR;
    errorPosition = targetpos - redGearPosition;
}

void funcEncoderB() // Encoder B에서 위치 받는 함수 (수업시간 참조)
{
    encA = gpioRead(ENCODERA);
    encB = gpioRead(ENCODERB);

    if (encB == 1) {
        if (encA == 0) encoderPosition--;
        else encoderPosition++;
    }

    else {
        if (encA == 0) encoderPosition++;
        else encoderPosition--;
    }

    redGearPosition = (float)encoderPosition / ENC2REDGEAR;
    errorPosition = targetpos - redGearPosition;
}


void PID() // PID 제어값 계산하는 함수
{
    errorPosition = (float)targetpos - redGearPosition;
    errorPositionBefore = (float)targetpos - redGearPosition;

    deltaTime = checkTime - checkTimeBefore;
    deltaError = errorPosition - errorPositionBefore;
    Integral += deltaTime * errorPosition;

    Pout = errorPosition * PGAIN;
    Iout = Integral * IGAIN;
    Dout = (deltaError / deltaTime) * DGAIN;

    PIDout = Pout + Iout + Dout;

    if (PIDout < 0) PIDout = -PIDout;

    scaledPIDout = PIDout * 2.55;

    scaledPIDoutint = (int) round(scaledPIDout); // gpioPWM용 int 값으로 바꿈

    if (scaledPIDoutint > 255) scaledPIDoutint = 255; // 0또는 255 값 초과시 오류 발생 방지
    if (scaledPIDoutint < 0) scaledPIDoutint = 0;

}

int main() 
{
    signal(SIGINT, handle_sigint);
    printf("Enter the CSV file name: ");
    scanf("%s", filename);
    filename_csv = filename;
    strcat(filename_csv, ".csv"); 
    file = fopen(filename_csv, "w+");

    if (file == NULL) {
        printf("Failed to open file\n");
        return 1;
    }
    
    if (gpioInitialise() < 0) {
        printf("Failed to initialize pigpio\n");
        return 1;
    }

    gpioSetMode(CLOCK, PI_INPUT);
    gpioSetMode(Pin1, PI_INPUT);
    gpioSetMode(Pin2, PI_INPUT);
    gpioSetMode(Pin3, PI_INPUT);

    gpioSetISRFunc(CLOCK, RISING_EDGE, 0, SubscribeClock); // ISRFunc 사용시 timeout 값 0으로 해도 괜찮은가요?

    gpioSetMode(ENCODERA,PI_INPUT);
    gpioSetMode(ENCODERB,PI_INPUT);

    gpioSetISRFunc(ENCODERA,EITHER_EDGE,0,funcEncoderA);
    gpioSetISRFunc(ENCODERB,EITHER_EDGE,0,funcEncoderB);

    gpioPWM(MOTOR1,255); 
    gpioPWM(MOTOR2,255);

    errorPosition = (float)targetpos - redGearPosition;
    errorPositionBefore = (float)targetpos - redGearPosition;

    startTime = gpioTick();  //gpioTick 함수 사용시 마이크로초로 계산됨 (millis()는 밀리초)
    checkTimeBefore = gpioTick();

    while(1) { 
        checkTime = gpioTick();
        if (checkTime - checkTimeBefore > T) { // 모터 움직이는 함수 (수업시간 참조)
            PID();
            if (errorPosition > 0) {
                gpioPWM(MOTOR1,scaledPIDoutint);
                gpioPWM(MOTOR2,0);
                printf("gearPos: %f errorPos: %f\n", redGearPosition, errorPosition);
            }

            else {
                gpioPWM(MOTOR2,scaledPIDoutint);
                gpioPWM(MOTOR1,0);
                printf("gearPos: %f errorPos: %f\n", redGearPosition, errorPosition);
            }

            updateDataArray();
            checkTimeBefore = checkTime;
        }
    }
    
    for (int p = 0; p < dataIndex; p++) { // 위치 저장용 배열
        fprintf(file, "%.3f,%.3f\n", dataArray[p][0], dataArray[p][1]);
    }

    fclose(file);
    gpioTerminate();
    return 0;
}