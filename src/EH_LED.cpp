#include "EH_LED.h"

/*!
    * @brief LEDの点灯モードを設定
    * @param mode :点灯モード
    * @param blink_period :点滅周期 1~199[CLK Cycle]
    * @return true:正常に設定 false:パラメタ異常（デフォルトを設定）
    */
bool EH_LED::setMode(E_IllumiMode mode, uint16_t period ) {
    LED_mode = mode;
    blink_flag = true;
    if (0 == period || period>200) {
        blink_period = DEFAULT_BLINK_PERIOD;
        blink_count = blink_period;
        return false;
    } else {
        blink_period = period;
        blink_count = blink_period;
    }
    return true;
};

/*!
 * @brief CLKに同期した処理を実行 物理的なLEDの操作
 *          リソースの占有・解放の処理
 *          処理時間：nucleo32 F303K8で2.5us程度
 */     
void EH_LED::clk_in(void){
    busy_now = true;

    if (LED_state) { //LED_state=true :点灯状態
        if (LED_mode==E_IllumiMode::CONT) {
            digitalWrite(LED_port_number, HIGH);
        } 
        else if (LED_mode==E_IllumiMode::BLINK) {
            if (--blink_count == 0){
                blink_count  = blink_period;
                blink_flag = !blink_flag;
            } 
            if (blink_flag) {
                digitalWrite(LED_port_number, HIGH);
            } else {
                digitalWrite(LED_port_number, LOW);
            }
        }
    } else {    //消灯状態
        digitalWrite(LED_port_number, LOW);
        blink_count = blink_period;
        blink_flag = true;
    }

    if(acquire_resource){
        busy_now = false;
        acquire_resource = false;
    }

    if(free_resource){
        if(--keep_busy==0){
            resource_available=true;
            free_resource=false;
            keep_busy=DEFAULT_KEEP_BUSY;
            // // for test
            // digitalWrite(LED_port_number, HIGH);
            // digitalWrite(LED_port_number, LOW);
            // // for test
        }
    }

    busy_now = false;    

};

/*!
* @brief リソースを占有する
* @param  void
* @return True:正常終了    False:現在使用中
*/
bool EH_LED::acquire(void){
    if(resource_available){
        acquire_resource = true;
        busy_now = true;    // 占有処理実行中なのでbusyをアサート
        resource_available = false;// リソースを占有したことを明示
        return true;
    } else {
        return false;
    }
};

/*!
* @brief リソースを解放する
* @param  void
* @return void
* @note 指定時間(DEFAULT_KEEP_BUSY)経過後解放される
*/
void EH_LED::free(void){
    free_resource=true;
    return;
};
