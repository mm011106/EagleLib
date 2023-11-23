#include "timeswitch.h"


/// @brief タイマの初期化（必須）
/// @param period タイマ周期の設定[min]
/// @return True:正常完了  False:タイマ周期指定エラー
/// @note 設定範囲は0-90で、１桁目は切り捨てられます @n ex: 12->10, 9->0, 57->50
bool TimeSwitch::init(const uint16_t period){
    // タイマの設定値は 0−90    それ以外はエラーを返す
    if (period > 90){
        if (DEBUG) {Serial.print("Timer too long");}
        return false;
    }   
    // 10分単位の値に整理（切り捨て）
    timer_period = (period / 10);
    timer_period = timer_period * 10;
    remaining_time = timer_period;
    if (DEBUG) {Serial.print("Timer set to:"); Serial.println(timer_period);}

    counter_10ms = timer_period * COUNTS_PER_MINUTES;  

    return true;
};

/// @brief CLKの入力 (100Hzを想定)
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
        if (--remaining_time == 0){
            remaining_time = timer_period;
        }
        if (DEBUG) {Serial.print("a minutes elapse: ");}
    }

    return;
};

/// @brief タイマが完了したかどうかを返します
/// @return True:完了  False:継続
/// @note タイマ計測開始のためのフラグです
bool TimeSwitch::hasOverflow(void){
    bool temp = overflow;
    overflow = false;
    return temp;
};

/// @brief 1分経過したかどうかを返します
/// @return True:1分経過   False:それ以外
/// @note 残り時間表示のためのフラグです
bool TimeSwitch::hasLapsedOneMinutes(void){
    bool temp = update;
    update = false;
    return temp;
};

/// @brief タイマが動作しているかを返します
/// @return True:動作   False:停止
bool TimeSwitch::isActive(void){
    return (timer_period != 0);
};


/// @brief タイマの残り時間を返します
/// @return 残り時間[min]
uint16_t TimeSwitch::getRemainingTime(void){
    return remaining_time;
};
