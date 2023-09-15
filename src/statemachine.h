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
        enum class ETransit : uint8_t{
            IDLE = 0,
            CLICK,  //  操作スイッチ  クリック
            LONG,   //  操作スイッチ  長押し
            TIMEUP, //  タイマ計測
            MEASCPL, //  計測ユニットからの測定完了信号（一回計測で計測が完了）
            MEASERR //  計測ユニットからの計測エラー信号
        };

        // @brief ステートマシン状態一覧
        enum class EStatus : uint8_t {
            TIMER = 0,
            MANUAL,
            CONT
        };

        /// @brief モード表示のための文字配列（EStatusと連携）
        const char ModeInd[3]={'T','M','C'};

        /// @brief 発行する計測コマンド一覧
        enum class EMeasCommand : uint8_t{
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

        bool setTransitSignal(ETransit signal);
        bool hasStatusUpdated(void);
        EMeasCommand getMeasCommand(void);
        EStatus getStatus(void);
        char getStatusChar(void);

    private:
        // debug flag
        constexpr static bool DEBUG = false;

        // Vars
        
        // 現在のステータスを保持
        EStatus machine_status = EStatus::TIMER;

        // 発行コマンド
        EMeasCommand command = EMeasCommand::IDLE;
 
        // 遷移命令を与えられたときの初期ステータスを保存する（遷移の有無を確認するため）
        EStatus previous_machine_status = EStatus::TIMER;

        // 状態遷移が行われたか
        bool updated = false;


};



#endif //_EH_statemachine_H_
