#include "EH_switch.h"


/*!
 * @brief CLKに同期した処理を実行 
 *          スイッチの押されている可動化の判断と押された時間の判断
 *          リソースの占有・解放の処理
 */
void EH_switch::clk_in(void){
    busy_now = true;
    // 全クロック処理   2.75us@64MHz

    uint16_t status = digitalRead(SW_port_number);

    // SWの押下検出  
    if (last_status != (status == HIGH)){
        if (status == HIGH){
        // 押された
            SW_state = true;
            pushed = true;
        } else {
        // 離された
            SW_state = false;
            released = true;
        }
    }
    // ステータス判断   処理時間:0.3us@64MHz
    if (SW_state){
        ++press_duration;
        if (press_duration > LONG_PRESS_TH){
            long_press = true;
            clicked = false;
        } else{
            long_press = false;
            clicked = true;
        }
    }

    if(acquire_resource){
        busy_now = false;
        acquire_resource = false;
    }

    if(free_resource){
        if(--keep_busy==0){
            resource_available=true;
            busy_now = true;//  解放されたことで動作できなくなるので、busyをアサート
            free_resource=false;
            keep_busy=DEFAULT_KEEP_BUSY;
        }
    }

    last_status = (status == HIGH);
    busy_now = false;    

};

/*!
* @brief リソースを占有する
* @param  void //戻り値をboolに
* @return True:正常終了    False:現在使用中
*/
bool EH_switch::acquire(void){
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
void EH_switch::free(void){
    free_resource=true;
    return;
};
