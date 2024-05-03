#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#define RX  3
#define TX  2
#define MAX_MSG 10

SoftwareSerial software_serial(RX, TX);

// IMPLEMENT MESSAGE ACKNOWLEDGEMENT ???
// IMPLEMENT QUEUE FOR BUSY ???
// AUTO CONNECT FEATURE ???
// DIFFERENTIATE IS_CONNECTED TO BHS_NET AND WIFI ???

  // ********************* ENUM ********************** //

    enum module_type : uint8_t {
      MAIN_MOD,
      ROOM_MOD,
      LEAF_MOD};

    struct Args {
      uint8_t argIndex       = 0;
      String  currentArg     = "";
      String* serial_args    = new String[10]();};

  // ******************* COMM PROCESS ******************** //

  class espManager {
    public:
      espManager();
      void receiveComms();
      void checkConnection();
    private:
    // METHODS //
    // INIT //
      void initialiseEsp(const String& _type, const String& _currentCode);
      void startSoftAP();
    // RECEIVE //
      bool serialParse(const char _c, Args& _args, bool sticky = false);
      void receiveServer();
      void receiveSerial();
      void receiveMaster();
    // EXEC //
      void execFromSerial(String*& _args);
      void execFromWifi(String*& _args);
    // MESSAGES //
      String arrayToString(String*& _args, bool discardFirstArg = false);
      void setMessageToMaster(const String& message);
      void addMessageToServer(const String& message);
      void removeMessageFromServer(const String& ID);
    // WIFI //
      void wifi_toggle(bool conn);

    // VARIABLES //
      bool isBusy         = false;
      bool _isConnected   = false;
      bool isInitialised  = false;
      bool firstBoot      = true;
      String* serverMessages = new String[MAX_MSG]();
      String messageToMaster = "";
      String currentCode;
      const String ssid = "BHS";
      const String pass = "testpassword";
      module_type type;
      //String date_time;
      //String _time;
      HardwareSerial* serialToModule;
      SoftwareSerial* serialFromModule;
      WiFiServer server = WiFiServer(80);
      //WiFiServerSecure server = WiFiServerSecure(443);
      //BearSSL::X509List* serverCertList = new BearSSL::X509List(cert);
      //BearSSL::PrivateKey* serverPrivKey = new BearSSL::PrivateKey(key);
  };
  
  espManager::espManager(){
    serialToModule = &Serial;
    serialFromModule = &software_serial;
    serialToModule->begin(9600);
    serialFromModule->begin(9600);
    /* server.setRSACert(serverCertList, serverPrivKey);*/}

  // *********************** INIT *********************** //
    
    void espManager::initialiseEsp(const String& _type, const String& _currentCode){
      type = _type=="0"?MAIN_MOD:ROOM_MOD;
      currentCode = _currentCode;
      startSoftAP();
      server.begin();
      isInitialised = true;
      serialToModule->print("OK\n");} 
    
    void espManager::startSoftAP(){ // DIFFERENTIATE IS_CONNECTED TO BHS_NET AND WIFI ???
      if(_isConnected) return;
      if(type == ROOM_MOD && isInitialised){
        IPAddress _ip = WiFi.localIP(); 
        _ip[2]=_ip[3]; _ip[3]= 1;
        WiFi.softAPConfig(_ip, _ip, IPAddress(255,255,255,0));}
      if(type == MAIN_MOD){
        wifi_set_opmode(STATIONAP_MODE);
          wifi_softap_dhcps_stop();
            struct ip_info info;
              IP4_ADDR(&info.ip, 192, 168, 0, 1);
              IP4_ADDR(&info.gw, 192, 168, 0, 1);
              IP4_ADDR(&info.netmask, 255, 255, 255, 0);
              wifi_set_ip_info(SOFTAP_IF, &info);
            struct dhcps_lease dhcp_lease;
              IP4_ADDR(&dhcp_lease.start_ip, 192, 168, 0, 1);
              IP4_ADDR(&dhcp_lease.end_ip, 192, 168, 0, 10);
              wifi_softap_set_dhcps_lease(&dhcp_lease);
          wifi_softap_dhcps_start();
          _isConnected = true;}
        WiFi.softAP(ssid, pass, 1, true, 8);}

  // *********************** RECEIVE *********************** //

    bool espManager::serialParse(const char _c, Args& _args, bool sticky){
      if(sticky && (_c == -1 || _c == 255)) return false;
      if(_c != -1 && _c != '.' && _c != ',' && _c != '\n' && _c != '\r' && _c!=255){
        _args.currentArg += _c; return false;}
      if(_args.currentArg == "") return false;
      _args.serial_args[_args.argIndex++] = _args.currentArg; 
      _args.currentArg = "";
      if(_c != 10 && _c != 13) return false;
      _args.argIndex = 0;
      return true;}
    
    void espManager::receiveMaster(){
      if(!_isConnected || type == MAIN_MOD) return;
      WiFiClient client;
      Args args;
      if(client.connect(IPAddress(192,168,WiFi.localIP()[2],1), 80)){
        if(messageToMaster != ""){
          client.print(messageToMaster+"\n");
          messageToMaster = "";}
        while(client.connected() || client.available())
          if(serialParse(char(client.read()), args, true)){
            execFromWifi(args.serial_args);
            if(client.available())
              args.serial_args = new String[10]();}}
      // ELSE PROBLEM WITH MASTER ???
      client.stop();}
    
    void espManager::receiveSerial(){
      if(serialFromModule->available()){
        Args args;
        while(true)
          if(serialParse(char(serialFromModule->read()), args, true)){
            execFromSerial(args.serial_args);
            break;}}}
    
    void espManager::receiveServer(){
      WiFiClient _client = server.accept();
      if(_client){
        Args args;
        while(_client.available()) 
          if(serialParse(char(_client.read()), args)){
            execFromWifi(args.serial_args);
            break;}
        for(uint8_t i=0; i<MAX_MSG; i++)
          if(serverMessages[i] != "")
            _client.print(serverMessages[i]+"\n");}}
          
    void espManager::receiveComms(){ 
      if(isBusy) return;
      receiveSerial();
      if(!isInitialised) return;
      if(millis()%2000==0) receiveMaster();
      receiveServer();}

  // *********************** EXEC *********************** //
    
    void espManager::execFromSerial(String*& _args){
        if(_args[0][0] == '0' || _args[0][0] == '1' || _args[0][0] == '2'){
          if(type)  setMessageToMaster("1."+arrayToString(_args));
          else      addMessageToServer("0."+arrayToString(_args));}
        else if(_args[0] == "INIT_ESP" &&_args[1] != "" && _args[2] != "")  initialiseEsp(_args[1], _args[2]);
        else if(_args[0] == "CODE" && _args[1] != "")  {currentCode = _args[1]; serialToModule->print("DONE\n");}
        else if(_args[0] == "RMV" && _args[1] != "")    removeMessageFromServer(_args[1]);    
        else if(_args[0] == "CONNECT" && type)  wifi_toggle(true);
        else if(_args[0] == "DISCONNECT")       wifi_toggle(false);
        else if(_args[0] == "WIFI_STAT")        serialToModule->print(_args[0]+"."+String((WiFi.status() == WL_CONNECTED) ? "1" : "0")+"\n");
        else if(_args[0] == "NET_STAT")         serialToModule->print(_args[0]+"."+String(_isConnected? "1" : "0")+"\n");
        else if(_args[0] == "POKE")             serialToModule->print("YES\n");
        //else if(_args[0] == "SYNC") syncClock();
        delete _args; _args = NULL;}
    
    void espManager::execFromWifi(String*& _args){
      if(type? _args[0]=="0" : true) 
        serialToModule->println(arrayToString(_args, true));
      delete _args; _args = NULL;}

  // *********************** WIFI *********************** //

    void espManager::wifi_toggle(bool conn){ // IMPLEMENT QUEUE FOR BUSY ???
      if(isBusy) {serialToModule->print("BUSY\n"); return;}
      if(conn && !_isConnected)       WiFi.begin("BHS", "testpassword"); 
      else if(!conn && _isConnected)  WiFi.disconnect(); 
      serialToModule->print("DONE\n");
      isBusy = true;}
    
    void espManager::checkConnection(){
      if(!isInitialised || type==MAIN_MOD) return;
      int _status = WiFi.status();
      if(!_isConnected && _status == WL_CONNECTED) {
        if(firstBoot){
          startSoftAP();
          serialToModule->print("CONNECT.OK\n");
          firstBoot = false;}
        else serialToModule->print("ESP.CONNECTED\n");
        isBusy = false;
        _isConnected = true;}
      else if(_isConnected && (_status==WL_CONNECT_FAILED || _status==WL_CONNECTION_LOST || _status==WL_DISCONNECTED)){
        if(isBusy){
          serialToModule->print("DISCONNECT.OK\n");
          isBusy = false;}
        else serialToModule->print("ESP.DISCONNECTED\n");
        _isConnected = false;}
      /*else if(!isBusy && WiFi.status() == WL_IDLE_STATUS) isBusy = true;*/}

  // *********************** MESSAGES *********************** //
    
    String espManager::arrayToString(String*& _args, bool discardFirstArg){
      String ret = "";
      for(uint8_t i = discardFirstArg? 1 : 0; i<10; i++){
        if(_args[i] == "") continue;
        if(i != (discardFirstArg? 1 : 0)) ret += ".";
        ret += _args[i];}
      return ret;}
    
    void espManager::addMessageToServer(const String& message){
      for(uint8_t i = 0; i<MAX_MSG; i++)
        if(serverMessages[i] == ""){
          serverMessages[i] = message; 
          serialToModule->print("DONE\n"); return;}
      serialToModule->print("OOM_MSG\n");}
    
    void espManager::setMessageToMaster(const String& message){
      messageToMaster = message; 
      serialToModule->print("DONE\n");}
    
    void espManager::removeMessageFromServer(const String& ID){
      for(uint8_t i = 0; i<MAX_MSG; i++)
        if(serverMessages[i].indexOf(ID) == 2/* && serverMessages[i].indexOf("0") == 0*/){
          serverMessages[i] = "";
          serialToModule->print("RMV.1\n"); return;}
      serialToModule->print("RMV.0\n");}
  
  // *********************** MAIN *********************** //

espManager* esp;

void setup(){
  esp = new espManager();}

void loop(){
  esp->checkConnection();
  esp->receiveComms();}
