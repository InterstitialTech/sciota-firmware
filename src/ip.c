bool ip_setup1(void) {

    if (!_send_confirm("AT+CGDCONT=1,\"IP\",\"soracom.io\"", "OK", 1000)) return false;

    if (!_send_confirm("AT+CGATT=1", "OK", 1000)) return false;

    if (!_send_confirm("AT+SAPBR=3,1,\"APN\",\"soracom.io\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+SAPBR=3,1,\"USER\",\"sora\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+SAPBR=3,1,\"PWD\",\"sora\"", "OK", 1000)) return false;
    if (!_send_confirm("AT+SAPBR=1,1", "OK", 1000)) return false;

    return true;

}

bool ip_send1(void) {

    char ATstring[30];
    char payload[30];

    // reset the HTTP session
    if (!_send_confirm("AT+HTTPINIT", "OK", 1000)) return false;

    // set the parameters
    if (!_send_confirm("AT+HTTPPARA=\"CID\",1", "OK", 1000)) return false;
    if (!_send_confirm("AT+HTTPPARA=\"URL\",\"http://demo.thingsboard.io/api/v1/K11HoE3QMPE7rSHPf3Hj/telemetry\"", "OK", 1000)) return false;
    //if (!_send_confirm("AT+HTTPPARA=\"URL\",\"76.16.217.80\"", "OK", 1000)) return false;
  	if (!_send_confirm("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", 1000)) return false;

    // prepare the data payload
    //sprintf(payload, "{\"temperature\": %.2f}", temperature);
    sprintf(payload, "{\"temperature\": 3.14}");
    sprintf(ATstring, "AT+HTTPDATA=%d,3000", strlen(payload));  // do i need *any* delay here?
    if (!_send_confirm(ATstring, "DOWNLOAD", 5000)) return false;
    if (!_send_confirm(payload, "OK", 6000)) return false;

    // send it!
    if (!_send_confirm("AT+HTTPACTION=1", "OK", 1000)) return false;
    if (!_confirm_response("+HTTPACTION: 1,200,0", 2000)) return false;

    if (!_send_confirm("AT+HTTPTERM", "OK", 1000)) return false;

    return true;

}

