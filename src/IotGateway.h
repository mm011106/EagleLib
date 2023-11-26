#ifndef _IOTGATEWAY_H_
#define _IOTGATEWAY_H_

#include "HardwareSerial.h"

class IotGateway : public HardwareSerial{

  public:
    /*!
    @brief  コンストラクタ 
    */
    IotGateway(uint32_t pin_rx, uint32_t pin_tx) : HardwareSerial(pin_rx, pin_tx){

    };

    void clk_in(void);

    void setPayload(const String& json);
    String getPayload(void);

    void addPayload(const String& key, const String& value);
    void addPayload(const String& key, const int32_t& value);
    void addPayload(const String& key, const float& value, const uint8_t& deciPlac);
    
    
    /*!
    @brief  payloadをクリア
    @param void
    @return void
    */
    void clearPayload(void){
      payload="";
    };
    
    /*!
    @brief  payloadをUARTに出力
    @param void
    @return void
    @note   payloadはクリアされます
    */
    void sendPayload(void){
      HardwareSerial::println(getPayload());
      clearPayload();
    };

  private:
    String payload;
    void joinToPayload(const String& node);
};

#endif // _IOTGATEWAY_H_