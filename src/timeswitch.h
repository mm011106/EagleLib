/**************************************************************************/
/*!
 * @file 
 * @brief 
 * @author 
 * @date 20230922
 * $Version:    0.0$
 * @par 
 *    
 * 
 */
/**************************************************************************/

#ifndef _TIMESWITCH_H_
#define _TIMESWITCH_H_

#include <Arduino.h>

class TimeSwitch {

    public:
    // consts
    
    // instances

    // vars

    
    // methods 
    /*!
    * @brief constructor  
    */
    TimeSwitch(){
    };

    /*!
    * @brief deconstructor
    *  
    */
    ~TimeSwitch(){
    };

    bool init(const uint16_t period = DEFAULT_TIMER_PERIOD); 
    void clk_in(void);

    bool hasOverflow(void);
    bool hasLapsedOneMinutes(void);
    bool isActive(void);

    uint16_t getRemainingTime(void);

    private:
    // consts

    // デフォルトのタイマ周期[min]
    static constexpr uint16_t DEFAULT_TIMER_PERIOD = 60;

    // 1分あたりのクロック数
    static constexpr uint16_t COUNTS_PER_MINUTES = 6000;

    // debug flag
    static constexpr bool DEBUG = true;

    // instances

    // vars

    //  タイマ周期設定値
    uint16_t timer_period = DEFAULT_TIMER_PERIOD; 

    // 10ms単位のダウンカウンタ
    uint32_t counter_10ms = 0;

    // 内部信号
    bool update = false;
    bool overflow = false;
    

};

#endif //_TIMESWITCH_H_