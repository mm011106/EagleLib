/**************************************************************************/
/*!
 * @file EH_switch.h
 * @brief  スイッチ入力管理クラス
 * @author Masakazu Miyamoto
 * @date 2022/7/11-
 * $Version:    1.0$
 * @par History
 *      2023/7/11   コーディング開始
 *      2023/7/12   V1.0 動作確認（ダブルクリック未実装）
 *      2023/8/22   V1.1 名前変更 EH_SW->EH_switch
 *
 */
/**************************************************************************/

#ifndef _EH_switch_H_
#define _EH_switch_H_

#include <Arduino.h>

/*!
 * @class   EH_switch
 * @brief   SWの入力を管理するクラス
 *          
 * 
 */

class EH_switch {
    public:

        /*!
         * @brief constructor   ポート番号を指定
         * @param port_number:  SWが接続されているポート番号（Arduino名もしくはSTM32ポート番号_ALT)
         * @note アクティブHIを想定している
         */
        EH_switch(uint32_t port_number){
            SW_port_number = port_number;
            pinMode(SW_port_number, INPUT);
        };
    
        /*!
         * @brief deconstructor
         *  使っていたポートを解放し、入力に設定。
         */
        ~EH_switch(){
            pinMode(SW_port_number, INPUT);
        };

        /*!
         * @brief  SWが接続されているポートを番号で返します
         * @param  void
         * @return uint32_t ulPin 
         */
        uint32_t getPortNumber(void){
            return SW_port_number;
        };


        void clk_in(void);

        bool acquire(void);
        void free(void);

        /*!
         * @brief リソースが命令受付可能かどうか
         * @return True:受け入れ可   False:命令実行中
         */
        bool isReady(void){
            return !busy_now;
        };

        /*!
         * @brief リソースを占有できる状態か？
         * @return True:占有可能   False:使用中
         */
        bool isAvailable(void){
            return resource_available;
        };

        /*!
         * @brief SWの内部状態を返します
         * @param  
         * @return  True:スイッチ押されている  False:スイッチ押されていない,
         */
        bool getStatus(void){
            return SW_state;
        };

        /*!
         * @brief SWが押されたかどうかを返します
         * @param  
         * @return True:押された    False:なにもされていない
         * @note 内部のフラグがリセットされます。スイッチ操作一回に対し一度しか使えません
         */
        bool isPushed(void){
            bool return_val = pushed;
            pushed = false;
            return return_val;
        }

        /*!
         * @brief SWが離されたかどうかを返します
         * @param  
         * @return True:離された    False:なにもされていない
         * @note 内部のフラグがリセットされます。スイッチ操作一回に対し一度しか使えません
         */
        bool isReleased(void){
            bool return_val = released;
            released = false;
            return return_val;
        }

        /// @brief スイッチの操作が通常のクリックかどうかを返す
        /// @return True:クリック
        /// @note isReleased()=Trueの場合に意味がある値を取ります
        bool isClicked(void){
            return clicked;
        }
        
        /// @brief スイッチの操作が長押しかどうかを返す
        /// @return True:長押し
        /// @note isReleased()=Trueの場合に意味がある値を取ります
        bool isLongPressing(void){
            return long_press;
        }

        /*!
         * @brief 内部の変数をクリアして次のイベントを準備します
         */ 
        void clearStatus(){
            last_status = false;
            SW_state = false;
            pushed = false;
            released = false;
            long_press = false;
            clicked = false;
            press_duration = 0;
            double_click_timer =0;
        }

    private:

        /*
        * スイッチ長押しの時間判定における判断基準[CLKサイクル]
        */
        static constexpr uint16_t LONG_PRESS_TH = 200;   //[CLK cycle]

        // リソース解放命令後の解放までの遅延時間設定（デフォルト）[CLKサイクル]
        static constexpr uint16_t DEFAULT_KEEP_BUSY = 2; //[CLK Cycle]

        // リソースのポート番号
        uint32_t SW_port_number = 0;

        // リソースが命令実行中
        bool busy_now = false;

        // リソースの占有の状態
        bool resource_available = true; 

        // リソースの占有を指示するフラグ
        bool acquire_resource = false;

        // リソースのリリースを指示するフラグ
        bool free_resource = false;

        /*!
         * @brief リソースをリリースした後も
         *        設定された時間だけビジーステートを有効にするための
         *        CLK用カウンタ
         *        [CLK count]
         */
        uint16_t keep_busy = DEFAULT_KEEP_BUSY;

        // 
        // 内部信号

        // 1クロック前のSW状態を示す True:押されていた  False:離されていた
        bool last_status = false;

        // 内部でのSW状態を示す True:押されている  False:離された状態
        bool SW_state = false;

        // SWの操作された状況を示す （読み出しメソッドを実行するとリセットされる）
        bool pushed = false;
        bool released = false;

        // 長押しと判断されているかどうか 
        // releasedがアサートされた時に判断される   True:長押しされた
        bool long_press = false;

        // 通常のクリックと判断されているかどうか 
        // releasedがアサートされた時に判断される   True:クリックされた 
        bool clicked = false;

        // swの押されている長さを測る [CLK]
        uint16_t press_duration = 0;

        //未実装 
        // ダブルクリックを判定するためのタイマ[CLK]
        uint16_t double_click_timer =0;

};



#endif //_EH_LED_H_
