/**************************************************************************/
/*!
 * @file statemachine.h
 * @brief EH-900 動作モード制御用ステートマシン
 * @author Masakazu Miyamoto
 * @date 2022/8/1-
 * $Version:    0.0$
 * @par History
 *    
 * 
 */
/**************************************************************************/

#ifndef _EH_statemachine_H_
#define _EH_statemachine_H_

#include <Arduino.h>

/*!
 * @class   Statemachine
 * @brief   EH-900 動作モード制御用ステートマシン
 *          
 * 
 */
class Statemachine {

    public:
        // Constatnts

        /// @brief ステートマシンの状態遷移イベント一覧
        enum class E_Transit : uint8_t{
            IDLE = 0,
            CLICK,  //  操作スイッチ  クリック
            LONG,   //  操作スイッチ  長押し
            TIMEUP, //  タイマ計測
            MEASCPL, //  計測ユニットからの測定完了信号（一回計測で計測が完了）
            MEASERR //  計測ユニットからの計測エラー信号
        };

        // @brief ステートマシン状態一覧
        enum class E_Status : uint8_t {
            TIMER = 0,
            MANUAL,
            CONT
        };

        /// @brief モード表示のための文字配列（E_Statusと連携）
        const char ModeInd[3]={'T','M','C'};

        /// @brief 発行する計測コマンド一覧
        enum class E_MeasCommand : uint8_t{
            IDLE = 0,
            START,
            STOP
        };

        // Methods

        /*!
         * @brief constructor   
         */
        Statemachine(){
        };
    
        /*!
         * @brief deconstructor
         *  
         */
        ~Statemachine(){
        };

        bool setTransitSignal(E_Transit signal);
        bool hasStatusUpdated(void);
        E_MeasCommand getMeasCommand(void);
        E_Status getStatus(void);
        char getStatusChar(void);

    private:
        // debug flag
        constexpr static bool DEBUG = false;

        // Vars
        
        // 現在のステータスを保持
        E_Status machine_status = E_Status::TIMER;

        // 発行コマンド
        E_MeasCommand command = E_MeasCommand::IDLE;
 
        // 遷移命令を与えられたときの初期ステータスを保存する（遷移の有無を確認するため）
        E_Status previous_machine_status = E_Status::TIMER;

        // 状態遷移が行われたか
        bool updated = false;


};



#endif //_EH_statemachine_H_
