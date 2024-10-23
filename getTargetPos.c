#include <stdio.h>
#include <pigpio.h>

// pin number는 그냥 임의의 숫자 넣어놨습니다:)
#define CLOCK 1
#define Pin1 11
#define Pin2 12
#define Pin3 13

int get_pos[3] = {};
int store_pos[5] = {};
int i = 0;
int j = 0;
int k = 0;

void SubscibeClock(void) // clock이 rising edge일때마다 호출되는 함수
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
            PublishTargetPos(store_pos[k]); // target position 반환함수 실행
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
                    PublishTargetPos(store_pos[k]); // target position 반환함수 실행
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

int PublishTargetPos(int targetpos) // targetpos를 반환하기 위한 함수 정의
{
    printf("target position : %d\n",targetpos);
    return targetpos;
}

int main(void)
{
    if (gpioInitialise() < 0) {
        printf("Failed to initialize pigpio\n");
        return 1;
    }

    gpioSetMode(CLOCK, PI_INPUT);
    gpioSetMode(Pin1, PI_INPUT);
    gpioSetMode(Pin2, PI_INPUT);
    gpioSetMode(Pin3, PI_INPUT);

    
    while (1) {
        gpioSetISRFunc(CLOCK, RISING_EDGE, 0, SubscibeClock);
    }

    gpioTerminate();
    return 0;
}