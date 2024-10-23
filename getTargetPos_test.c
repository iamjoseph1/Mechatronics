#include <stdio.h>

int num = 0;
int get_pos[3] = {};
int store_pos[5] = {};
int i = 0;
int j = 0;
int k = 0;

int PublishTargetPos(int targetpos) // targetpos를 반환하기 위한 함수 정의
{
    printf("target position : %d\n",targetpos);
    return targetpos;
}

int main(void)
{
    
    while (1) {
        printf("enter integer(0~7) : ");
        scanf("%d",&num);

        if (i == 3){
            get_pos[0] = get_pos[1];
            get_pos[1] = get_pos[2];
            get_pos[2] = num;

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
            get_pos[i] = num;
            i++;
            switch (i)
            {
            case 2:
                if (get_pos[0] == get_pos[1] && j > 0) // store_pos에 저장된 target position이 있으면서 첫 2개 원소가 0일 때
                {   
                    if (get_pos[0] == 0){
                        PublishTargetPos(store_pos[k]); // target position 반환함수 실행
                        k++;
                    }
                }
                break;
            case 3:
                if (get_pos[0] == get_pos[1] && get_pos[1] == get_pos[2]){
                    store_pos[j] = -get_pos[0];
                    printf("%d stored!\n", store_pos[j]);
                    j++;
                    i = 0; get_pos[1] = 0; get_pos[2] = 0;// 중복 탐지를 피하기 위해 배열 초기화
                }
                else if (get_pos[0] == get_pos[1]){
                    if (get_pos[0] != 0){
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
    return 0;
}