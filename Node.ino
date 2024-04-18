#define DATA_MAX_LEN 120
#define MODULE_TYPE  MAIN_NODE
#define MODULE_ID    12345678
#define MAX_MODULES  20
#define MAX_FEATURE  20
#define MAX_ROOM     5
#define MAX_LEAF     10
#define MAX_NODE     10
#define MAX_STATE    10
#define MAX_COND     10

#include <time.h>
#include <AES.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

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
      LEAF_MOD
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
    };
    
    struct Data {
      String data;
      time_t _time;
    };
    
    class DataRecords {
      private:
        Data* records = new Data[DATA_MAX_LEN]{{"Hello"},{"Hi"}};
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
      Unit* module;
      Feature(String __name, feature_type _type, Unit* _module) 
        {_name = __name; module = _module; type = _type;}
    };

    struct Actuator : Feature {
      uint8_t currentState = 0; // index of possible states "green"/"red"/"yellow"
      bool isOverriden = false;
      State** states = new State*[MAX_STATE]();
      Actuator(Unit* _module, String __name, State** _states, feature_type _type = ACTUATOR) 
      : Feature(__name, _type, _module) 
        {states = _states;}
    };

    struct Sensor : Feature {
      uint8_t queryInterval;
      Actuator** actuatorsLinked = new Actuator*[MAX_LEAF]();
      DataRecords data;
      Sensor(Unit* _module, String __name, uint8_t _queryInterval, feature_type _type = SENSOR) 
      : Feature(__name, _type, _module) 
        {queryInterval =_queryInterval;}
    };
    
  // ********************* MODULES ********************** //
    
    class Module {
      public:
        Module(module_type _type, Unit* _Unit, String _address = {}, Module** _modules = nullptr, Feature** _features = nullptr);
        Module();
        String address;
        String ID;
        String code;
        String room_name = "";
        Module* master = nullptr;
        Module** modules = nullptr;
        Feature** features = nullptr;
        module_type type;
      protected:
        bool enabled = false;
    };

  // ********************* MANAGERS ********************** //
    
    class CommManager {
      public:
        CommManager(comm_type _type, Unit* _currentUnit);
        Module** initDeviceMappingOfType(module_type _type, Unit** _units, String current_id = "");
        // bool execCommProcesses();
        String* receiveSerial(bool sticky = false, uint8_t numberOfArg = 1, uint8_t numberOfChar = 1);
        String getDeviceAddress(module_type _type, String ID);
        String* sendMessageTo(Module*, String, uint8_t numberOfArg = 1);
      private:
        void closeComm();
        bool openComm();
        HardwareSerial* serial;
        Unit* currentUnit;
        //CommProcess* currentComms[MAX_MODULES] = {};
        uint8_t comm_index = 0;
        bool isInitialised = false;
        comm_type type;
        //String public_name;
    };

    class AppManager : CommManager { // WE HAVE AN APP MANAGER HERE /////////////////////////////////////////////////
      public:
        AppManager(String _device);
        // bool refreshApp();
        String address;
    };
    
    class MemoryManager {
      public:
        MemoryManager(module_type _type);
        bool initFromMemory();
        uint8_t generateUnit(module_type _type, String device_ID);
        bool deleteUnit(module_type _type, String device_ID);
        Unit* getUnitByCode(String _code, Unit* _units);
        Unit* getUnitByID(String _ID, Unit* _units);
        bool checkMemory();
        Unit* rooms = new Unit[MAX_NODE];  // GETTERS
        Unit* leafs = new Unit[MAX_NODE];  // HERE
        Unit* master_unit = new Unit[1];   // ???
      private:
        module_type type;
        bool isInitialised = false;
        byte numberOfRooms;
        byte numberOfLeafs;
        byte lastRoom;
        byte lastLeaf;
    };
    
    
    class ModuleManager : public Module {
      public:
        ModuleManager(module_type _type, Unit* _Unit, String _address = {}, Module** _modules = nullptr, Feature** _features = nullptr);
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
        void execCommand(String* _args);
        MemoryManager* memory;
    };
  
  
  






/* ************************************************************* */
/*                         DEFINITION                            */
/* ************************************************************* */
  
  
  // ********************* MODULE ********************** //
    
    Module::Module(module_type _type, Unit* _Unit, String _address, Module** _modules, Feature** _features){
      type =     _type;
      ID =       _Unit->ID;
      code =     _Unit->code;
      address =  _address;
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
      if(type==_WIFI){
        Serial1.begin(9600);
        serial = &Serial1;
        serial->print("REQUEST_READY\n"); delay(5000);
        if(receiveSerial()[0] != "OK"){
          Serial.println("  WiFi:        ERROR"); return;}
        isInitialised = true;
        Serial.println("  WiFi:        OK");}
      else{
        Serial2.begin(115200);
        serial = &Serial2;
        if(!openComm()){
          Serial.println("  Bluetooth:   ERROR");
          return;}
        closeComm();
        isInitialised = true;
        Serial.println("  Bluetooth:   OK");}}

    Module** CommManager::initDeviceMappingOfType(module_type _type, Unit** _units, String current_id){
      if(!isInitialised) return false;
      unsigned long previousMillis;
      Module** modules = new Module*[MAX_MODULES]();
      uint8_t module_index = 0;
      String address = "";
      Serial.println(String(type==0?"WiFi ": "Bluetooth ")+"Manager - Initiating "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping -");
      for(uint8_t i = 0; i<MAX_NODE; i++){
        if(_units[0]==nullptr) continue;
        Serial.println("  Attempting to connect with "+String(_type==0?"Master ":_type==1?"Room ":"Leaf ")+_units[0][i].ID);
        if(type==_BLUETOOTH){
          address = getDeviceAddress(_type, _units[0][i].ID);
          if(address == ""){
            Serial.println("    Confirmation Failed : Restart Scan or Delete Device?");
            continue;}}
        Module* module = new Module(_type, &_units[0][i], address);
        String* args = sendMessageTo(module, "INIT_CONFIRM"+String(_type==MAIN_MOD?"."+current_id:""), 2);
        if(args[1]=="INIT_OK"){
          Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Confirmed");
          if(args[2] != "") module->features = ModuleManager::parseFeatures(args[2]);
          modules[module_index]= module;
          module_index++;}
        else Serial.println("  Confirmation Failed : Restart Scan or Delete Device?");}
      if(module_index != sizeof(*_units)) 
            Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping Failure");
      else  Serial.println("  "+String(_type==0?"Master":_type==1?"Room":"Leaf")+" Device Mapping Success");
      return modules;}

    String CommManager::getDeviceAddress(module_type _type, String ID){
      String address = "";
      String* args;
      uint8_t counter = 0;
      uint8_t numberOfFound;
      openComm();
      serial->print("IQ\r");
      receiveSerial(true,3);
      args = receiveSerial(true);
      if(args[0].indexOf("Found ")==0){ 
        numberOfFound = args[0].substring(6).toInt();
        do{
          if(counter==numberOfFound) args = receiveSerial(true);
          else                       args = receiveSerial(true,4);
          if(args[0].length()==12){
            counter++;
            if(args[1]=="BHS"+ID){
              address = args[0];
              Serial.println("  Successful address retrieval");}}}
        while(args[0].indexOf("I")!=0);}
      closeComm();
      if(address=="") Serial.println("  Failed to retrieve address");
      return address;}

    String* CommManager::sendMessageTo(Module* _module, String message, uint8_t numberOfArg){
      if(!isInitialised) return {};
      Serial.println("    Sending message: '"+message+"' to "+_module->ID);
      if(type==_BLUETOOTH){
        openComm();
        serial->print("C,"+_module->address+"\r");
        if(receiveSerial(true,1,3)[0] != "TRYING"){
          closeComm(); return {};}
        if(receiveSerial(true,1)[0] != "%CONNECT"){
          Serial.println("      Connection Failed");
          closeComm(); return {};}
        receiveSerial(true,2);}
      serial->print(String(_module->type?_module->code:currentUnit->code)+"."+message+"\n"); delay(500);
      String* ret = receiveSerial(true, numberOfArg);
      Serial.println("      Communication Successful!");
      if(type==_BLUETOOTH){
        serial->print("$$$"); delay(100);
        if(receiveSerial()[0] != "CMD") return {};
        serial->print("K,1\r"); 
        receiveSerial(true);
        receiveSerial(true);}
      return ret;}

//    bool CommManager::execCommProcesses(){
//      unsigned long previousMillis = millis();
//      for(uint8_t i = 0; i<sizeof(currentComms); i++){
//        if(currentComms[i]){
//          
//        }
//      }
//    }
    
    String* CommManager::receiveSerial(bool sticky, uint8_t numberOfArg, uint8_t numberOfChar){
      uint8_t argIndex       = 0;
      String* serial_args    = new String[10];
      String currentArg      = "";
      while(sticky? (serial_args[numberOfArg-1]==nullptr || serial_args[numberOfArg-1].length()<numberOfChar) : serial->available()){
        char c = serial->read();
        //Serial.print(c);
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
      
    void CommManager::closeComm(){
      do{serial->print("---\r"); delay(100);}
      while(receiveSerial(true)[0] != "END");}
    
    bool CommManager::openComm(){
      unsigned long previousMillis = millis();
      serial->print("$$$"); delay(100);
      if(receiveSerial()[0] != "CMD"){
        do{
          serial->print("$\r");
          if(receiveSerial()[0] == "?") return true;}
        while(millis()>previousMillis+5000);
        return false;}
      return true;}
  // ********************* MEMORY MANAGER ********************** //
    
    MemoryManager::MemoryManager(module_type _type){
      type = _type;
      if(EEPROM[EEPROM.length()-1] == 1) initFromMemory();
      else Serial.print("Not initialised\n");}
    
    bool MemoryManager::initFromMemory(){
      Unit** Unit_arr = new Unit*[2]();
      Serial.print("Initialising from memory -\n");
      isInitialised = true;
      if (type == MAIN_MOD){
        numberOfRooms = EEPROM[0];
        Serial.println("    Number of Rooms: "+String(numberOfRooms));
        Unit_arr[0] = rooms;}
      if (type == MAIN_MOD || type == ROOM_MOD){
        numberOfLeafs = EEPROM[1];
        Serial.println("    Number of Leafs: "+String(numberOfLeafs));
        Unit_arr[1] = leafs;}
      if (type == ROOM_MOD || type == LEAF_MOD){
        Serial.println("    Master code found");
        master_unit[0] = {String(EEPROM[100])+String(EEPROM[101]), String(EEPROM[102])};}
      for(uint8_t j = 0; j<2; j++)
        if(Unit_arr[j] != nullptr)
          for(uint8_t i = 100*(j+1); EEPROM[i]!=255; i=i+3){
            Serial.println("  Found "+String(j==0?"Room ":"Leaf ")+String(EEPROM[i])+String(EEPROM[i+1])+":"+String(EEPROM[i+2]));
            Unit_arr[j][((i-100*(j+1)+3)/3)-1] = 
              {String(EEPROM[i])+String(EEPROM[i+1]), String(EEPROM[i+2])};}
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
        code = random(0,255);
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
    
    ModuleManager::ModuleManager(module_type _type, Unit* _Unit, String _address, Module** _modules, Feature** _features) 
    : Module(_type, _Unit, _address, _modules, _features) {
      Serial.println("Initiating module -");
      Serial.println("  Module type: "+String(type==0? "MAIN_MOD": type==1? "ROOM_MOD": "LEAF_MOD"));
      if (type == MAIN_MOD || type == ROOM_MOD)
        wifi = new CommManager(_WIFI, _Unit);
      bt     = new CommManager(_BLUETOOTH, _Unit);
      memory = new MemoryManager(_type);
      checkExistingConnections();}
    
    void ModuleManager::checkSystem(){
      memory->checkMemory();}

    void ModuleManager::checkExistingConnections(){
      if (type == LEAF_MOD)
        updateModules(bt->initDeviceMappingOfType(ROOM_MOD, &memory->master_unit));
      if (type == MAIN_MOD){
        updateModules(wifi->initDeviceMappingOfType(ROOM_MOD, &memory->rooms));
        updateModules(wifi->initDeviceMappingOfType(LEAF_MOD, &memory->leafs));}
      if (type == ROOM_MOD)
        updateModules(wifi->initDeviceMappingOfType(MAIN_MOD, &memory->master_unit, "BSH"+ID));
      updateModules(bt->initDeviceMappingOfType(LEAF_MOD, &memory->leafs));}

    void ModuleManager::updateModules(Module** _modules){
      if(_modules!=nullptr)
        for(uint8_t i=0; i<MAX_MODULES; i++)
          for(uint8_t j=0; j<MAX_MODULES; j++)
            if(modules[j]==nullptr && _modules[i]!=nullptr){
              if(_modules[i]->type == MAIN_MOD) master = _modules[i];
              modules[j] = _modules[i]; break;}}
              
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

    void ModuleManager::execCommand(String* _args){
      Module* module = getModuleByCode(_args[0]);
      if(type==MAIN_MOD){
        /* **************** IRR COMMANDS ***************** */
        // if(_args[1] == "INPUT"){}
        // if(_args[1] == "DATA"){}
        if(_args[1] == "INIT_CONFIRM" && module != nullptr)
          sendMessageTo(module, "INIT_OK");
        if(_args[1] == "INIT_OK" && module != nullptr)
          updateFeatures(parseFeatures(_args[2])); // DO THAT §§§§§§§§§§§
        }
      else if(type==ROOM_MOD){
        if(_args[1] == "INIT_CONFIRM" && _args[0] == master->ID) 
          sendMessageTo(master, "INIT_OK."+getFeaturesJSON());
      }
      else if(type==LEAF_MOD){
        if(_args[1] == "INIT_CONFIRM" && _args[0] == master->ID) 
          sendMessageTo(master, "INIT_OK."+getFeaturesJSON());
      }
      
      /* ********************************************* */}
      
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
    
    bool ModuleManager::sendMessageTo(Module* _module, String message){
      if(_module==nullptr) return;
      if(_module->type==ROOM_MOD || _module->type==MAIN_MOD)
        wifi->sendMessageTo(_module, message);
      if(_module->type==LEAF_MOD)
        bt->sendMessageTo(_module, message);}

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

/* ************************************************************* */
/*                              MAIN                             */
/* ************************************************************* */

ModuleManager* manager;
Unit* coreUnit = new Unit {"000999","000"};
// INITIALISE FEATURES/SENSORS/ACTUATORS HERE

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
    {NO_DEP},
    {"temperature.>=.30"}}
  };

Feature* aaafeatures[MAX_FEATURE] = {
  new Sensor{coreUnit, "temperature", 30},
  new Actuator{coreUnit, "fan", fanStates}};

// ************************ MAIN ************************ //

void setup(){
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial2.begin(115200);
    Serial.print("BOOTING\n\n");
    manager = new ModuleManager(MAIN_MOD, coreUnit, "Get address from Btmod", nullptr, aaafeatures);
    //manager->checkSystem(); // will may be there
    Serial.print("\nMODULE READY\n\n");
    delay(1000);

      // CHECK TO SEE HOW TO TRANSFER FEATRUES JSON DATA // PUT THAT IN A FUNCTION
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, manager->getFeaturesJSON());
    serializeJsonPretty(doc, Serial);
    
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

  //if(manager->bt != nullptr) manager->bt->receiveSerial();
  //if(manager->wifi != nullptr) manager->wifi->receiveSerial();

  //  String str;
  //  while(Serial.available()){
  //    Serial2.print(Serial.readString());}
  //  while(Serial2.available()){
  //    Serial.print("OUT! "+Serial2.readString());}
  //manager->bt->sendMessageTo(new ModuleManager(MAIN_MOD, {"200201","203"}, "A0:B1:C3:D4"), );
  }



// MAIN
//    EEPROM[0] = 2;
//    EEPROM[1] = 2;
//    EEPROM[100] = 100;
//    EEPROM[101] = 101;
//    EEPROM[102] = 102;
//
//    EEPROM[103] = 103;
//    EEPROM[104] = 104;
//    EEPROM[105] = 105;
//
//    EEPROM[200] = 200;
//    EEPROM[201] = 201;
//    EEPROM[202] = 202;
//
//    EEPROM[203] = 203;
//    EEPROM[204] = 204;
//    EEPROM[205] = 205;
//
//    EEPROM[LAST] = 1/255; //isInit

// ROOM
//    EEPROM[0] = 1/255;
//    EEPROM[1] = 2;
//
//    EEPROM[200] = 200;
//    EEPROM[201] = 201;
//    EEPROM[202] = 202;
//
//    EEPROM[203] = 203;
//    EEPROM[204] = 204;
//    EEPROM[205] = 205;
//
//    EEPROM[LAST-2] = 000;
//    EEPROM[LAST-1] = 123;
//    EEPROM[LAST] = 102;

// LEAF
//    EEPROM[0] = 1/255;
//    EEPROM[1] = ###; // OPTIONAL BLACKBOX
//
//    EEPROM[200] = ###; // OPTIONAL BLACKBOX
//    EEPROM[201] = ###; // OPTIONAL BLACKBOX
//    EEPROM[202] = ###; // OPTIONAL BLACKBOX
//
//    EEPROM[LAST-2] = 000;
//    EEPROM[LAST-1] = 123;
//    EEPROM[LAST] = 202;

  
