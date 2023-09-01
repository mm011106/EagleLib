#include "statemachine.h"


/*!
    * @brief 状態遷移のためのトリガ信号を与えます
    * @param  Statemachine::ETransit型   状態遷移のための信号名
    * @return true: statusが更新された false: 更新なし
    * @note 計測コマンド(command)もこの中でセットされる
    */
bool Statemachine::setTransitSignal(ETransit signal){

  // for test
  Serial.print("(Signal:");
  // 
  previous_machine_status = machine_status;
  bool update = true;  

  switch (signal) {
    case ETransit::CLICK :
      Serial.print("CLICK)");
      if(machine_status == EStatus::TIMER){
        machine_status = EStatus::MANUAL;
        command = EMeasCommand::START;
      }
      // else if(machine_status == EStatus::MANUAL){
      //   machine_status = EStatus::TIMER;
      // }
      else if(machine_status == EStatus::CONT){
        machine_status = EStatus::TIMER;
        command = EMeasCommand::STOP;
      } else {
        update = false;
        command = EMeasCommand::IDLE;
        }
    break;

    case ETransit::LONG :
      Serial.print("LONG)");
      if (machine_status == EStatus::TIMER){
        machine_status = EStatus::CONT;
        command = EMeasCommand::START;
      } else if (machine_status == EStatus::CONT){
        machine_status = EStatus::TIMER;
        command = EMeasCommand::STOP;
      } else {
        update = false;
        command = EMeasCommand::IDLE;
        }   
    break;
      
    case ETransit::TIMEUP :
      Serial.print("TIMEUP)");
      if(machine_status == EStatus::TIMER){
        machine_status = EStatus::MANUAL;
        command = EMeasCommand::START;
      } else {
        update = false;
        command = EMeasCommand::IDLE;
        }   
    break;
      
    case ETransit::MEASCPL :
      Serial.print("MEAS end)");
      if(machine_status == EStatus::MANUAL){
        machine_status = EStatus::TIMER;
        command = EMeasCommand::STOP;
      } else {
        update = false;
        command = EMeasCommand::IDLE;
        }   
    break;

    case ETransit::IDLE :
      Serial.print("IDLE)");
      update = false;
      command = EMeasCommand::IDLE;
    break;

    default:
      update = false;
      command = EMeasCommand::IDLE;
    break;
  }
  
  // for test
  // Serial.println(static_cast<uint8_t>(machine_status));
  // 

  Statemachine::updated = (previous_machine_status != machine_status);

  return (updated);
}

/*!
    * @brief 状態遷移が行われたかどうかを返します
    * @return true: statusが更新された false: 更新なし
    * @note 読み取るたびに値はクリアされます
    */
bool Statemachine::hasStatusUpdated(void){
    bool temp_flag = updated;
    updated = false;
    return temp_flag;
};

/// @brief 状態遷移に応じた計測部への指示を返します
/// @return EMeasCommand型 測定命令
/// @note hasStatusUpdated()==true 時に有効になります
Statemachine::EMeasCommand Statemachine::getMeasCommand(void){
    return command;
}

/*!
    * @brief マシンの内部状態を返します
    * @param  
    * @return EStatus型 
    */
Statemachine::EStatus Statemachine::getStatus(void){
    return machine_status;
};

/// @brief ステートマシンの状態に応じた文字を返します（表示用）
/// @param  void
/// @return ModeIndで定義した文字の中の1文字が状態に応じて返されます
char Statemachine::getStatusChar(void){
    return ModeInd[static_cast<uint8_t>(machine_status)];
}
