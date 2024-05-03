#define DATA_MAX_LEN 120
#define MAX_MODULES  20
#define MAX_FEATURE  20
#define MAX_ROOM     5
#define MAX_LEAF     10
#define MAX_NODE     10
#define MAX_STATE    10
#define MAX_COND     10
#define MAX_MSG      10

#include <time.h>
#include <AES.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

IF IT'S 0 IT MEANS IT'S GOING DOWN SO MASTER IS SPEAKING TO YOU, 1 ONLY MEANS YOU PASS PN THE MESSAGE

/* ************************************************************* */
/*                           HEADERS                             */
/* ************************************************************* */

  // ********************* ENUM ********************** //
    
    enum data_type : uint8_t {
      NUMBER,
      STRING,
    };
    
    enum comm_type : uint8_t {
      _WIFI,
      _BLUETOOTH
    };
    
    enum comm_state : uint8_t {
      _CONNECTING,
      _CONNECTED,
      _NONE
    };
    
    enum module_type : uint8_t {
      MAIN_MOD,
      ROOM_MOD,
      LEAF_MOD,
      NO_MOD
    };

    enum dependence_type : uint8_t {
      NO_DEP,
      NOT,
      AND,
      OR,
      XAND,
      XOR,
      NAND,
      NOR,
    };

    enum feature_type : uint8_t {
      SENSOR,
      ACTUATOR
    };
  
  // ********************* DATA TYPES ********************** //
    
    struct Unit{
      String ID;
      String code;
      String address;
    };
    
    struct Data {
      String data;
      time_t _time;
    };
    
    class DataRecords {
      private:
        Data* records = new Data[DATA_MAX_LEN]();
        uint8_t front = 0;
        DataRecords &operator= (const DataRecords&) = delete;
        DataRecords(const DataRecords&) = delete;
      public:
        void push(Data _record){
          records[front] = _record;
          front == DATA_MAX_LEN-1 ? front = 0 : front++;}
        Data* all()    {return records;}
        Data latest() {return front ? records[front-1] : records[DATA_MAX_LEN-1];}
        data_type d_type;
        String measurementUnit;
        DataRecords() = default;
    };

    struct State {
      String _name;
      dependence_type dep_type[MAX_COND]; // "AND","AND"
      String conditions[MAX_COND]; // N[code or ID]/["temperature"].["=","!=",">","<","<=",">="].[value]
    };

    struct Feature {
      String _name;
      feature_type type;
      bool isEnabled = false;
      Unit* module = nullptr;
      Feature(String __name, feature_type _type) 
        {_name = __name; type = _type;}
    };

    struct Actuator : Feature {
      uint8_t currentState = 0; // index of possible states "green"/"red"/"yellow"
      bool isOverriden = false;
      State** states = new State*[MAX_STATE]();
      Actuator(String __name, State** _states, feature_type _type = ACTUATOR) 
      : Feature(__name, _type) 
        {states = _states;}
    };

    struct Sensor : Feature {
      uint8_t queryInterval;
      Actuator** actuatorsLinked = new Actuator*[MAX_LEAF]();
      DataRecords data;
      Sensor(String __name, uint8_t _queryInterval, feature_type _type = SENSOR) 
      : Feature(__name, _type)
        {queryInterval =_queryInterval;}
    };
    
  // ********************* MODULES ********************** //
    
    class Module {
      public:
        Module(Unit* _Unit, Module** _modules = nullptr, Feature** _features = nullptr);
        //Module();
        String address;
        String ID;
        String code;
        String room_name = "";
        Unit* unit = nullptr;
        Module* master = nullptr;
        Module** modules = nullptr;
        Feature** features = nullptr;
        module_type type;
      protected:
        bool enabled = false;
    };

  // ********************* MANAGERS ********************** //
    
    struct Message{
      Unit* unit;
      String message;
      uint8_t numberOfArg;
    };
    
    class CommManager {
      public:
        CommManager(comm_type _type, Unit* _currentUnit);
        Module** initDeviceMappingOfType(module_type _type, Unit** _units, String current_id = "");
        // bool execCommProcesses();
        String* receiveSerial(bool sticky = false, uint8_t numberOfArg = 1, uint8_t numberOfChar = 1);
        String getDeviceAddress(module_type _type, String ID);
        String* sendMessageTo(Unit*, String, uint8_t numberOfArg = 1);
        void connectToNetwork();
        void checkPendingMessages();
        bool isConnectedToNetwork = false;
        bool isBusy = false;
      private:
        void closeComm();
        bool openComm();
        HardwareSerial* serial;
        Unit* currentUnit;
        Unit* master_unit; // NECESSARY???? FOR SENDING TO MASTER
        Message** pendingMessages = new Message*[MAX_MSG]();
        //CommProcess* currentComms[MAX_MODULES] = {};
        uint8_t comm_index = 0;
        bool isInitialised = false;
        comm_type type;
        //String public_name;
    };

    class AppManager : CommManager { // WE HAVE AN APP MANAGER HERE // THIS USES BLUETOOTH, NOT WIFI!!! //
      public:
        AppManager(String _device);
        // bool refreshApp();
        String address;
    };
    
    class MemoryManager {
      public:
        MemoryManager();
        uint8_t generateUnit(module_type _type, String device_ID);
        bool deleteUnit(module_type _type, String device_ID);
        Unit* getUnitByCode(String _code, Unit* _units);
        Unit* getUnitByID(String _ID, Unit* _units);
        bool checkMemory();
        Unit* coreUnit    = nullptr;  // GETTERS
        Unit* master_unit = nullptr;  // ???
        Unit** rooms = new Unit*[MAX_NODE]();  // GETTERS
        Unit** leafs = new Unit*[MAX_NODE]();  // HERE
        module_type type;
      private:
        bool isInitialised = false;
        byte numberOfRooms;
        byte numberOfLeafs;
        byte lastRoom;
        byte lastLeaf;
    };
    
    
    class ModuleManager : public Module {
      public:
        // ModuleManager(module_type _type, Unit* _Unit = nullptr, String _address = {}, Module** _modules = nullptr, Feature** _features = nullptr);
        ModuleManager(Feature** _features = nullptr);
        void checkSystem();
        void initSystem();
        void checkExistingConnections();
        void updateModules(Module** _modules);
        void updateFeatures(Feature** features);
        static Feature** parseFeatures(String _features);
        Module* getModuleByCode(String _code);
        Module* getModuleByID(String _ID);
        Sensor* getSensor(String _feature);
        Actuator* getActuator(String _feature);
        Feature* getFeature(String _feature);
        String getFeaturesJSON();
        bool sendMessageTo(Module* _module, String message);
        void receiveComm();
    //    void initConn(); // For Room/Leaf
    //    void requestAllow(); // For Room
    //    void allowRoom(); // For Main/Room
    //    void allowLeaf(); // For Main/Room
    //  private:
    //    bool verifyType(Module*, module_type);
    //    bool acceptRoom; // For Main
    //    bool acceptLeaf; // For Main/Room
        CommManager* bt;   // For Main/Room/Leaf
        CommManager* wifi; // For Main/Room
        MemoryManager* memory;
        void execCommand(String* _args);
    };
  
  
  






/* ************************************************************* */
/*                         DEFINITION                            */
/* ************************************************************* */
  
  
  // ********************* MODULE ********************** //
    
    Module::Module(Unit* _unit, Module** _modules, Feature** _features){
      unit = _unit;
      ID =       _unit==nullptr? "" : _unit->ID;
      code =     _unit==nullptr? "" : _unit->code;
      address =  _unit==nullptr? "" : _unit->address;
      type =     ID[0]=='0'? MAIN_MOD : ID[0]=='1'? ROOM_MOD : LEAF_MOD;
      modules =  _modules==nullptr? new Module*[MAX_MODULES]() : _modules;
      features = _features==nullptr? new Feature*[MAX_FEATURE]() : _features;
      }

//  // ******************* COMM PROCESS ******************** //
//  
//    CommProcess::CommProcess(String _destination, String _message, comm_process_type _type){
//      destination = _destination;
//      messages[message_index++] = _message;
//      type = _type;}
  
  // ********************* COMM MANAGER ********************** //

    CommManager::CommManager(comm_type _type, Unit* _currentUnit){
      type = _type;
      currentUnit = _currentUnit;
      String* _args = NULL;
      if(type==_WIFI){
        serial = &Serial1;
        serial->begin(9600);
        serial->print("REQUEST_READY."+String(_currentUnit->ID[0])+"."+String(_currentUnit->code)+"\n"); delay(200);
        _args = receiveSerial(true);
        if(_args[0] != "OK"){
          delete _args; _args = NULL;
          Serial.println("  WiFi:        ERROR"); return;}
        isInitialised = true;
        if(currentUnit->ID[0]!='0'){
          serial->print("CONNECT.\"BHS\".\"testpassword\"\n"); delay(200);
          _args = receiveSerial(true,2);
          if(_args[1] != "OK"){
            delete _args; _args = NULL;
            Serial.println("  WiFi:        INITIALISED"); return;}
          isConnectedToNetwork = true;
          Serial.println("  WiFi:        CONNECTED"); return;}
        Serial.println("  WiFi:        INITIALISED");}
      else{
        serial = &Serial2;
        serial->begin(115200);
        if(!openComm()){
          Serial.println("  Bluetooth:   ERROR");
          return;}
        closeComm();
        isInitialised = true;
        Serial.println("  Bluetooth:   OK");}}

    Module** CommManager::initDeviceMappingOfType(module_type _type, Unit** _units, String current_id){
      if(!isInitialised) return;
      unsigned long previousMillis;
      Module** modules = new Module*[MAX_MODULES]();
      String* _args = NULL;
      uint8_t module_index = 0;
      uint8_t counter = 0;
      String address = "";
      Serial.println(String(type==0?"WiFi ": "Bluetooth ")+"Manager - Initiating "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping -");
      for(uint8_t i = 0; i<MAX_NODE; i++){
        if(_units[i] == nullptr) continue;
        counter++;
        Serial.println("  Attempting to connect with "+String(_type==0?"Master ":_type==1?"Room ":"Leaf ")+_units[0][i].ID);
        if(type==_BLUETOOTH){
          address = getDeviceAddress(_type, _units[i]->ID);
          if(address == ""){
            Serial.println("    Confirmation Failed"); continue;}
          _units[i]->address = address;}
        if(currentUnit->code == "000"){ // WHEN INIT_CONFIRM 000 => CHECK IF BOTH ID+CODE IS CURRENTUNIT? MAIN : ELSE
          if(_type) _args = sendMessageTo(_units[i], "INIT_CONFIRM."+_units[i]->ID, type==_WIFI?0:2);
          else      _args = sendMessageTo(currentUnit, "INIT_CONFIRM."+currentUnit->ID, type==_WIFI?0:2);}
        else  _args = sendMessageTo(currentUnit, "INIT_CONFIRM."+_units[i]->ID, type==_WIFI?0:2);
        if(_args[1]=="INIT_OK"){
          Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Confirmed");
          Module* module = new Module(&_units[0][i]);
          if(_args[2] != "") module->features = ModuleManager::parseFeatures(_args[2]);
          modules[module_index++] = module;}
        else if(type == _WIFI && _args[0]=="DONE") Serial.println("  WiFi Confirmation Broadcasted Successfully : Waiting for reply...");
        else {
          Serial.println("  Confirmation Failed : Restart Scan or Delete Device?");
          _units[i]->address = "";}
        delete _args; _args = NULL;}
      if(module_index != counter && type != _WIFI)
            Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping Failure");
      else  Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping Success");
      return modules;}

    String CommManager::getDeviceAddress(module_type _type, String ID){
      String address = "";
      String* _args = NULL;
      uint8_t counter = 0;
      uint8_t numberOfFound;
      openComm();
      serial->print("IQ\r");
      delete receiveSerial(true,3);
      _args = receiveSerial(true);
      if(_args[0].indexOf("Found ")==0){ 
        numberOfFound = _args[0].substring(6).toInt();
        do{
          delete _args; _args = NULL;
          if(counter==numberOfFound) _args = receiveSerial(true);
          else                       _args = receiveSerial(true,4);
          if(_args[0].length()==12){
            counter++;
            if(_args[1]=="BHS"+ID){
              address = _args[0];
              Serial.println("  Successful address retrieval");}}}
        while(_args[0].indexOf("I")!=0);}
      delete _args; _args = NULL;
      closeComm();
      if(address=="") Serial.println("  Failed to retrieve address");
      return address;}

    String* CommManager::sendMessageTo(Unit* _unit, String message, uint8_t numberOfArg){
      if(!isInitialised) return nullptr;
      if(isBusy)
        for(uint8_t i=0; i<MAX_MSG; i++){
          if(pendingMessages[i] != nullptr) continue;
          pendingMessages[i] == new Message{_unit, message, numberOfArg};
          return nullptr;} // WRITE CHECK PENDING MESSAGES => FREE MEMORY AFTER CONSUME !!!
      String* _args = NULL;
      Serial.println("    Sending message: '"+message+"' to "+_unit->ID);
      if(type==_BLUETOOTH){
        openComm();
        serial->print("C,"+_unit->address+"\r");
        _args = receiveSerial(true,1,3);
        if(_args[0] != "TRYING"){
          Serial.println("      Connection Failed");
          delete _args; _args = NULL;
          closeComm(); return {};}
        delete _args; _args = NULL;
        _args = receiveSerial(true,1);
        if(_args[0] != "%CONNECT"){
          Serial.println("      Connection Failed");
          delete _args; _args = NULL;
          closeComm(); return nullptr;}
        delete receiveSerial(true,2);}
      if(currentUnit->ID[0]==_unit->ID[0]+1){
        serial->print(((currentUnit->code!="")?String(currentUnit->code+"."):"")+message+"\n");
        Serial.print(((currentUnit->code!="")?String(currentUnit->code+"."):"")+message+"\n");}
      else{
        serial->print(_unit->code+"."+message+"\n");
        Serial.print(_unit->code+"."+message+"\n");
      }
      delay(500);
      String* ret = receiveSerial(true, numberOfArg);
      Serial.println("      Communication Successful!");
      if(type==_BLUETOOTH){
        serial->print("$$$"); delay(100);
        _args = receiveSerial();
        if(_args[0] != "CMD"){
          delete _args; _args = NULL;
          return nullptr;}
        serial->print("K,1\r"); 
        delete receiveSerial(true);
        delete receiveSerial(true);}
      return ret;}
    
    String* CommManager::receiveSerial(bool sticky, uint8_t numberOfArg, uint8_t numberOfChar){
      uint8_t argIndex       = 0;
      String* serial_args    = new String[10];
      String currentArg      = "";
      while(sticky? (serial_args[numberOfArg-1]==nullptr || serial_args[numberOfArg-1].length()<numberOfChar) : serial->available()){
        char c = serial->read();
        Serial.print(c);
        if(c != -1 && c != '.' && c != ',' && c != '\n' && c != '\r' && c!=255){
          delay(10);
          //Serial.print(c);
          currentArg += c; continue;}
        if(currentArg == "") continue;
        serial_args[argIndex++] = currentArg; currentArg = "";
        if((serial->peek()==-1 || serial->peek()==255 || serial->peek()==13 || serial->peek()==10) && serial_args[numberOfArg-1].length()>=numberOfChar) break;}
      //Serial.println();
      //Serial.println("Sending " + serial_args[0] + "-" + serial_args[1] + "-" + serial_args[2] + "-" + serial_args[3]+ "-" + serial_args[4]);
      return serial_args;}

    void CommManager::checkPendingMessages(){
      if(type!=_WIFI) return;
      for(uint8_t i=0; i<MAX_MSG; i++){
        if(pendingMessages[i] == nullptr) continue;
        sendMessageTo(pendingMessages[i]->unit, pendingMessages[i]->message, pendingMessages[i]->numberOfArg);
        delete pendingMessages[i]; pendingMessages[i] == NULL;}}

    void CommManager::connectToNetwork(){
      if(type !=_WIFI) return;
      serial->print("CONNECT.\"BHS\".\"testpassword\"\n");
      isBusy = true;}
      
    void CommManager::closeComm(){
      String* _args = NULL;
      do{delete _args; _args = NULL;
        serial->print("---\r"); delay(100);
        _args = receiveSerial(true);}
      while(_args[0] != "END");
      delete _args; _args = NULL;}
    
    bool CommManager::openComm(){
      unsigned long previousMillis = millis();
      String* _args = NULL;
      serial->print("$$$"); delay(100);
      _args = receiveSerial();
      if(_args[0] != "CMD"){
        do{delete _args; _args = NULL;
          serial->print("$\r");
          _args = receiveSerial(true);
          if(_args[0] == "?"){
            delete _args; _args = NULL;
            return true;}}
        while(millis()>previousMillis+5000);
        delete _args; _args = NULL;
        return false;}
      delete _args; _args = NULL;
      return true;}
      
  // ********************* MEMORY MANAGER ********************** //
    
    MemoryManager::MemoryManager(){
      Unit*** unit_arr = new Unit**[2]();
      Serial.print("Initialising from memory -\n");
      isInitialised = true;
      String str1 = String(EEPROM[EEPROM.length()-1]<10?"00":EEPROM[EEPROM.length()-1]<100?"0":"")+String(EEPROM[EEPROM.length()-1]);
      String str2 = String(EEPROM[EEPROM.length()-2]<10?"00":EEPROM[EEPROM.length()-2]<100?"0":"")+String(EEPROM[EEPROM.length()-2]);
      String str3 = String(EEPROM[EEPROM.length()-3]<10?"00":EEPROM[EEPROM.length()-3]<100?"0":"")+String(EEPROM[EEPROM.length()-3]);
      coreUnit = new Unit{str1+str2, str3, ""};
      type = ((str1[0] == '0')? MAIN_MOD : ((str1[0] == '1')? ROOM_MOD : LEAF_MOD));
      Serial.println("  Module type: "+String(type==0? "MAIN_MOD": type==1? "ROOM_MOD": "LEAF_MOD"));
      if (type == MAIN_MOD && EEPROM[0] != 255){
        numberOfRooms = EEPROM[0];
        unit_arr[0] = rooms;
        Serial.println("    Number of Rooms: "+String(numberOfRooms));}
      if ((type == MAIN_MOD || type == ROOM_MOD) && EEPROM[1] != 255){
        numberOfLeafs = EEPROM[1];
        unit_arr[1] = leafs;
        Serial.println("    Number of Leafs: "+String(numberOfLeafs));}
//      if ((type == ROOM_MOD || type == LEAF_MOD) && EEPROM[0]==1){
//        master_unit = new Unit{String(EEPROM[100])+String(EEPROM[101]), String(EEPROM[102])};
//        Serial.println("    Master found");}
      for(uint8_t j = 0; j<2; j++)
        if(unit_arr[j] != nullptr)
          for(uint8_t i = 100*(j+1); EEPROM[i]!=255; i=i+3){
            String str1 = String(EEPROM[i]<10?"00":EEPROM[i]<100?"0":"")+String(EEPROM[i]);
            String str2 = String(EEPROM[i+1]<10?"00":EEPROM[i+1]<100?"0":"")+String(EEPROM[i+1]);
            String str3 = String(EEPROM[i+2]<10?"00":EEPROM[i+2]<100?"0":"")+String(EEPROM[i+2]);
            Serial.println("  Found "+String(j==0?"Room ":"Leaf ")+str1+str2+":"+str3);
            unit_arr[j][((i-100*(j+1)+3)/3)-1] = new Unit{str1+str2, str3, ""};}
      Serial.print("  DONE\n");}
    
    uint8_t MemoryManager::generateUnit(module_type _type, String device_ID){
      uint8_t code;
      byte* numberOfType;
      byte* lastOfType;
      if(_type == ROOM_MOD){
           numberOfType = &numberOfRooms;
           lastOfType = &lastRoom;}
      else{numberOfType = &numberOfLeafs;
           lastOfType = &lastLeaf;}
      while(numberOfType != 255){
        code = random(255);
        for(uint8_t i=0; i>numberOfType; i++) if(EEPROM[(_type == ROOM_MOD? 100 : 200)+i+2] == code) continue;
        break;}
      if(*lastOfType == *numberOfType){
        EEPROM[(_type == ROOM_MOD? 100+numberOfRooms : 200+numberOfLeafs)]= device_ID.substring(0,2).toInt();
        EEPROM[(_type == ROOM_MOD? 100+numberOfRooms+1 : 200+numberOfLeafs+1)]= device_ID.substring(3,6).toInt();
        EEPROM[(_type == ROOM_MOD? 100+numberOfRooms+2 : 200+numberOfLeafs+2)]= code;
        *numberOfType++;
        *lastOfType++;}
      else{
        for(uint8_t i=0; i>numberOfType; i++)
          if(EEPROM[(_type == ROOM_MOD? 100 : 200)+i] == 255){
            EEPROM[(_type == ROOM_MOD? 100+i : 200+i)]= device_ID.substring(0,2).toInt();
            EEPROM[(_type == ROOM_MOD? 100+i+1 : 200+i+1)]= device_ID.substring(3,6).toInt();
            EEPROM[(_type == ROOM_MOD? 100+i+2 : 200+i+2)]= code;}
        *numberOfType++;}
      return code;}
    
    bool MemoryManager::deleteUnit(module_type _type, String device_ID){
      byte* numberOfType;
      byte* lastOfType;
      if(_type == ROOM_MOD){    
           numberOfType = &numberOfRooms;
           lastOfType = &lastRoom;}
      else{numberOfType = &numberOfLeafs;
           lastOfType = &lastLeaf;}
      for(uint8_t i=0; i>numberOfType; i++)
        if(EEPROM[(_type == ROOM_MOD? 100 : 200)+i] == device_ID.substring(0,2).toInt() && EEPROM[(_type == ROOM_MOD? 100 : 200)+i+1] == device_ID.substring(3,5).toInt()){
          EEPROM[(_type == ROOM_MOD? 100+numberOfRooms : 200+numberOfLeafs)]= 255;
          EEPROM[(_type == ROOM_MOD? 100+numberOfRooms+1 : 200+numberOfLeafs+1)]= 255;
          EEPROM[(_type == ROOM_MOD? 100+numberOfRooms+2 : 200+numberOfLeafs+2)]= 255;
          if(i==*numberOfType-1) *lastOfType--;
          *numberOfType--;
          return true;}
      Serial.print("Error while deleting Unit : No device found");
      return false;}
    
    bool MemoryManager::checkMemory(){
      /* COMPARISON BETWEEN SYS VALUES AND EEPROM VALUES */
    }

    Unit* MemoryManager::getUnitByCode(String _code, Unit* _units){
      for(uint8_t i =0; i<MAX_MODULES; i++)
        if(_units[i].code != _code)
        return &_units[i];
      return nullptr;}

    Unit* MemoryManager::getUnitByID(String _ID, Unit* _units){
      for(uint8_t i =0; i<MAX_MODULES; i++)
        if(_units[i].ID != _ID)
        return &_units[i];
      return nullptr;}
  
  // ********************* MODULE MANAGER ********************** //
    
    ModuleManager::ModuleManager(Feature** _features)
    : Module(nullptr, nullptr, _features) {
      Serial.begin(9600);
      Serial.println("BOOTING\n");
      Serial.println("Initiating module -");
      memory = new MemoryManager();
      type = memory->type;
      for(uint8_t i = 0; i<MAX_FEATURE; i++)
        if(features[i] != nullptr) features[i]->module = memory->coreUnit;
      if (type == MAIN_MOD || type == ROOM_MOD)
        wifi = new CommManager(_WIFI, memory->coreUnit);
      bt     = new CommManager(_BLUETOOTH, memory->coreUnit);
      checkExistingConnections();
      Serial.print("\nMODULE READY\n\n");}

    void ModuleManager::checkSystem(){
      if(wifi!=nullptr){
        if(millis()%30000==0 && !wifi->isBusy && !wifi->isConnectedToNetwork)
          wifi->connectToNetwork();
        wifi->checkPendingMessages();}
      memory->checkMemory();}

    // *** INITIALISATION *** //

    void ModuleManager::checkExistingConnections(){
      if (type == LEAF_MOD){
        Unit** ptr = new Unit*[MAX_NODE]{memory->coreUnit};
        updateModules(bt->initDeviceMappingOfType(ROOM_MOD, ptr));
        delete ptr; ptr = NULL;}
      if (type == MAIN_MOD){
        updateModules(wifi->initDeviceMappingOfType(ROOM_MOD, memory->rooms));
        updateModules(wifi->initDeviceMappingOfType(LEAF_MOD, memory->leafs));}
      if (type == ROOM_MOD){
        Unit** ptr = new Unit*[MAX_NODE]{memory->coreUnit};
        updateModules(wifi->initDeviceMappingOfType(MAIN_MOD, ptr));
        delete ptr; ptr = NULL;}
      updateModules(bt->initDeviceMappingOfType(LEAF_MOD, memory->leafs));}

    // *** COMMANDS *** //

    void ModuleManager::execCommand(String* _args){
      Module* module = getModuleByCode(_args[0]);
      if(type==MAIN_MOD){
        /* **************** IRR COMMANDS ***************** */
        // if(_args[1] == "INPUT"){}
        if(_args[0] == "CONNECT"){
          if(_args[1] == "OK") wifi->isConnectedToNetwork = true;
          wifi->isBusy = false;}
        if(_args[1] == "INIT_CONFIRM" && module != nullptr)
          //if(_args[0]=="000" && _args[2][0]!='0')
          //  memory->generateUnit():
          sendMessageTo(module, "INIT_OK");
        if(_args[1] == "INIT_OK" && module != nullptr)
          updateFeatures(parseFeatures(_args[2])); // DO THAT §§§§§§§§§§§
        }
      else if(type==ROOM_MOD){
        if(_args[1] == "INIT_CONFIRM" && _args[0] == master->ID)
          sendMessageTo(master, "INIT_OK."+getFeaturesJSON());
        if(_args[1]>='0'&&_args[1]<='9') sendMessageTo(getModuleByCode(_args[1]), _args[2]);}
      else if(type==LEAF_MOD){
        if(_args[1] == "INIT_CONFIRM" && _args[0] == master->code)
          sendMessageTo(master, "INIT_OK."+getFeaturesJSON());
        if(_args[1] == "DATA" && _args[0] == code)
          sendMessageTo(master, "SomeData");
      }
      
      /* ********************************************* */}

    // *** COMM INTERFACE *** //

    void ModuleManager::receiveComm(){
      if(bt != nullptr){
        if(type==ROOM_MOD)  sendMessageTo(master, bt->receiveSerial()[1]); //MAKE STRING*ARR TO STR?????
        if(type==LEAF_MOD)  execCommand(bt->receiveSerial());}
      if(wifi != nullptr)   execCommand(wifi->receiveSerial());}
    
    bool ModuleManager::sendMessageTo(Module* _module, String message){
      if(_module==nullptr) return;
      if(type==MAIN_MOD || type==ROOM_MOD){
        if(_module->type==MAIN_MOD || _module->type==ROOM_MOD)
          wifi->sendMessageTo(_module->unit, message);
        if(_module->type==LEAF_MOD)
          bt->sendMessageTo(_module->unit, message);}
      else if(type==LEAF_MOD)
        bt->sendMessageTo(_module->unit, message);}

    // *** FEATURES *** //

    Actuator* ModuleManager::getActuator(String _feature){
      for(uint8_t i = 0; i<10; i++)
        if(features[i] != nullptr && features[i]->_name == _feature) 
          return features[i];
      return nullptr;}

    Sensor* ModuleManager::getSensor(String _feature){
      for(uint8_t i = 0; i<10; i++)
        if(features[i] != nullptr && features[i]->_name == _feature) 
          return features[i];
      return nullptr;}

    Feature* ModuleManager::getFeature(String _feature){
      for(uint8_t i = 0; i<10; i++){
        if(features[i] == nullptr) continue;
        if(features[i]->_name == _feature){
          Actuator* curr = features[i];
          return curr;}
        if(features[i]->_name == _feature){
          Sensor* curr = features[i];
          return curr;}}}

    Feature** ModuleManager::parseFeatures(String _features){
      // Use the ARDUINO JSON
    }

    void ModuleManager::updateFeatures(Feature** features){
      // Same as Modules Probably
    }

    String ModuleManager::getFeaturesJSON(){
      String rep = "{\"features\":[{";
      for(uint8_t i = 0; i<MAX_FEATURE; i++){
        if(features[i] == nullptr) continue;
        if(i) rep+=",{";
        rep+= "\"name\":\""+features[i]->_name+"\",";
        rep+= "\"module\":[\""+features[i]->module->ID+"\",\""+features[i]->module->code+"\"],";
        rep+= "\"isEnabled\":"+String(features[i]->isEnabled)+",";
        rep+= "\"type\":"+String(features[i]->type)+",";
        if(features[i]->type){ // Actuator
          Actuator* curr = features[i];
          rep+= "\"currentState\":"+String(curr->currentState)+",";
          rep+= "\"isOverriden\":"+String(curr->isOverriden)+",";
          rep+= "\"state\":[{";
          for(uint8_t j=0; j<MAX_STATE; j++){
            if(curr->states[j] == nullptr) continue;
            if(j) rep+=",{";
            rep+= "\"name\":\""+curr->states[j]->_name+"\",";
            rep+= "\"dep_type\":[";
            for(uint8_t k=0; k<MAX_COND; k++){
              if(curr->states[j]->dep_type[k] == 0) continue;
              if(k) rep+= ",";
              rep+= String(curr->states[j]->dep_type[k]);}
            rep+= "],\"conditions\":[";
            for(uint8_t k=0; k<MAX_COND; k++){
              if(curr->states[j]->conditions[k] == "") continue;
              if(k) rep+= ",";
              rep+= "\""+curr->states[j]->conditions[k]+"\"";}
            rep+= "]}";}
          rep+= "]";}
        else{ // Sensor
          Sensor* curr = features[i];
          rep+= "\"queryInterval\":"+String(curr->queryInterval)+",";
          rep+= "\"actuatorsLinked\":[";
          for(uint8_t j=0; j<MAX_COND; j++){
            if(curr->actuatorsLinked[j] == nullptr) continue;
            if(j) rep+= ",";
            rep+= curr->actuatorsLinked[j]->module->code;}
          rep+= "]";}
        rep+= "}";}
      return rep;}

    // *** MODULES *** //

    Module* ModuleManager::getModuleByCode(String _code){
      for(uint8_t i =0; i<MAX_MODULES; i++)
        if(modules[i] != nullptr && modules[i]->code != _code)
        return modules[i];
      return nullptr;}

    Module* ModuleManager::getModuleByID(String _ID){
      for(uint8_t i =0; i<MAX_MODULES; i++)
        if(modules[i] != nullptr && modules[i]->ID != _ID)
        return modules[i];
      return nullptr;}

    void ModuleManager::updateModules(Module** _modules){
      if(_modules!=nullptr)
        for(uint8_t i=0; i<MAX_MODULES; i++)
          for(uint8_t j=0; j<MAX_MODULES; j++)
            if(modules[j]==nullptr && _modules[i]!=nullptr){
              if(_modules[i]->type == MAIN_MOD) master = _modules[i];
              modules[j] = _modules[i]; break;}}







/* ************************************************************* */
/*                              MAIN                             */
/* ************************************************************* */

ModuleManager* manager;

void clear_mem(){
  EEPROM[0] = 255;
  EEPROM[1] = 255;
  EEPROM[100] = 255;
  EEPROM[101] = 255;
  EEPROM[102] = 255;
  EEPROM[103] = 255;
  EEPROM[104] = 255;
  EEPROM[105] = 255;
  EEPROM[200] = 255;
  EEPROM[201] = 255;
  EEPROM[202] = 255;
  EEPROM[203] = 255;
  EEPROM[204] = 255;
  EEPROM[205] = 255;
  EEPROM[EEPROM.length()-1] = 255;}

// MAIN {"000999","000"}
void main_mem(){
  EEPROM[0] = 1;
  EEPROM[1] = 1;
  EEPROM[100] = 101;
  EEPROM[101] = 123;
  EEPROM[102] = 179;
  EEPROM[200] = 201;
  EEPROM[201] = 56;
  EEPROM[202] = 203;
  EEPROM[EEPROM.length()-1] = 0;
  EEPROM[EEPROM.length()-2] = 1;
  EEPROM[EEPROM.length()-3] = 0;}

// ROOM {"101123","179"} memory->master_unit
void room_mem(){
  EEPROM[0] = 1/255;
  EEPROM[1] = 1;
  EEPROM[200] = 201;
  EEPROM[201] = 56;
  EEPROM[202] = 203;
  EEPROM[EEPROM.length()-1] = 101;
  EEPROM[EEPROM.length()-2] = 123;
  EEPROM[EEPROM.length()-3] = 179;}

// LEAF {"201056","203"}
void leaf_mem(){
  EEPROM[0] = 1/255;
  EEPROM[100] = 101;
  EEPROM[101] = 123;
  EEPROM[102] = 179;
  EEPROM[EEPROM.length()-1] = 201;
  EEPROM[EEPROM.length()-2] = 56;
  EEPROM[EEPROM.length()-3] = 203;}
//    (unlikely)
//    EEPROM[1] = ###; // OPTIONAL BLACKBOX
//    EEPROM[200] = ###; // OPTIONAL BLACKBOX
//    EEPROM[201] = ###; // OPTIONAL BLACKBOX
//    EEPROM[202] = ###; // OPTIONAL BLACKBOX



State* fanStates[MAX_STATE] = {
  new State{
    "fanOff",
    {NO_DEP},
    {"temperature.<.20"}},
  new State{
    "fanSlow",
    {AND},
    {"temperature.>=.25",
      "temperature.<.30"}},
  new State{
    "fanFast",
    {},
    {"temperature.>=.30"}}
  };

Feature* coreFeatures[MAX_FEATURE] = {
  new Sensor{"temperature", 30},
  new Actuator{"fan", fanStates}};








// ************************ MAIN ************************ //

void setup(){
  delay(1000);
  manager = new ModuleManager(coreFeatures);

  //clear_mem();
  //main_mem();
  //room_mem();
  //leaf_mem();
  
      // CHECK TO SEE HOW TO TRANSFER FEATRUES JSON DATA // PUT THAT IN A FUNCTION
//    StaticJsonDocument<200> doc;
//    DeserializationError error = deserializeJson(doc, manager->getFeaturesJSON());
//    serializeJsonPretty(doc, Serial);
    
      // FEATURE CONDITIONS ACTUATOR SENSOR TEST
    //Serial.println(manager->getActuator("fan")->states[1]->_name); 
    // CREATE WRAPPER TO USE EASILY GETFEATURE FOR BOTH ACTUATORS AND SENSORS???
    //for(uint8_t i = 0; i<10; i++)
      //if(manager->features[i] != nullptr && manager->features[i]->_name == "fan")
        //{Actuator* curra = manager->features[i];
        //Serial.println(curra->states[0]->_name);
        //Serial.println(manager->getActuator("fan")->states[1]->_name);
        //Actuator* currb = manager->features[i];
        //Serial.println(currb->states[2]->_name);}

      //SEND MESSAGE TEST
    //Serial.println("comm test");
    //manager->bt->sendMessageTo(manager->getModuleByCode("202"), "Custom message here, haha Hi!");
    //manager->wifi->sendMessageTo(new Module{ROOM_MOD, new Unit{"123456","789"}}, "SOMECOMMAND");
    
    // Make func to getModule/getRoom/getLeaf
    }

void loop(){
  manager->checkSystem();
  manager->receiveComm();
//    String str = "";
//    // TEST TALK TO BT
//    while(Serial.available()){
//      Serial2.println(Serial.readString());}
//    // TEST READ BT
//    while(Serial2.available()){
//      Serial.print(char(Serial2.read()));}

}
