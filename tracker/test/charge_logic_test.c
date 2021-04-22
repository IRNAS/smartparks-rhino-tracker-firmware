/******************************************************************************
Test logic of the charging
*******************************************************************************/

#include <stdio.h>

enum charging_e{
  DISABLED,
  UNDERVOLTAGE,
  CYCLE_DISCHARGE,
  CYCLE_CHARGE,
  CHARGE
};

enum charging_e charging_state=UNDERVOLTAGE;

int input_voltage = 10000;
int system_input_charge_min= 9000;
int battery = 3800;
int system_charge_max = 4000;
int system_charge_min = 3600;

int main()
{
    printf("Hello World\n");
    
    for(int i = 0;i<100;i++){
        // undervoltage charging protection
        if(charging_state==DISABLED){
          charging_state=DISABLED;
        }
        else if(input_voltage<system_input_charge_min){
          charging_state=UNDERVOLTAGE;
          printf("UNDERVOLTAGE\n");
        }
        else{
          // cycle charging between two thresholds for battery voltage
          if((battery>system_charge_max) && system_charge_max>0){
            // disable charging as voltage is greater then threshold
            charging_state=CYCLE_DISCHARGE;
            printf("CYCLE_DISCHARGE\n");
          }
          /// cycle charging between two thresholds for battery voltage
          else if((battery<system_charge_min) && system_charge_min>0){
            // disable charging as voltage is greater then threshold
            charging_state=CYCLE_CHARGE;
            printf("CYCLE_CHARGE\n");
          }
          else if((charging_state!=CYCLE_CHARGE) & (charging_state!=CYCLE_DISCHARGE)){
            charging_state=CHARGE;
            printf("CHARGE\n");
          }
        }
        printf("%d %d\n",battery, charging_state);
    
        if(charging_state==DISABLED){
            battery+=-50;
        }
        else if(charging_state==CYCLE_DISCHARGE){
            battery+=-50;
        }
        else{
            battery+=50;
        }
    }
    return 0;
}



