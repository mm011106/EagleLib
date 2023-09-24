#include "timeswitch.h"

bool TimeSwitch::init(const uint16_t period){
    // タイマの設定値は 0−90    それ以外はエラーを返す
    if (period > 90){
        if (DEBUG) {Serial.print("Timer too long");}
        return false;
    }   
    // 10分単位の値に整理（切り捨て）
    timer_period = (period / 10);
    timer_period = timer_period * 10;
    
    if (DEBUG) {Serial.print("Timer set to:"); Serial.println(timer_period);}

    counter_10ms = timer_period * COUNTS_PER_MINUTES;  

    return true;
};

void TimeSwitch::clk_in(void){
    // タイマを動作させるかどうかの判断
    if (timer_period == 0){
        return;
    }

    // タイマのカウントダウン
    if (--counter_10ms == 0){
        counter_10ms = timer_period * COUNTS_PER_MINUTES;
        overflow = true;
        if (DEBUG) {Serial.println("Timer overflow: ");}
    }
    
    // 1分ごとに「更新」フラグを立てる
    if ((counter_10ms % COUNTS_PER_MINUTES)==0){
        update = true;
        if (DEBUG) {Serial.print("a minutes elapse: ");}
    }

    return;
};

bool TimeSwitch::hasOverflow(void){
    bool temp = overflow;
    overflow = false;
    return temp;
};

bool TimeSwitch::hasLapsedOneMinutes(void){
    bool temp = update;
    update = false;
    return temp;
};


bool TimeSwitch::isActive(void){
    return (timer_period != 0);
};

uint16_t TimeSwitch::getRemainingTime(void){
    return (uint16_t)(counter_10ms / COUNTS_PER_MINUTES);
};
