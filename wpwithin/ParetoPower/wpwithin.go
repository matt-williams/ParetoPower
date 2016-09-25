package ParetoPower

import (
  "encoding/json"

  "github.com/wptechinnovation/worldpay-within-sdk/sdkcore/wpwithin"
  "github.com/wptechinnovation/worldpay-within-sdk/sdkcore/wpwithin/types"
  //"github.com/wptechinnovation/worldpay-within-sdk/sdkcore/wpwithin/types/event"
)

type Wrapper interface {
  AddService(service *types.Service) error
  RemoveService(service *types.Service) error
  InitConsumer(scheme, hostname string, portNumber int, urlPrefix, clientID, hceCard string) error
  InitProducer(merchantClientKey, merchantServiceKey string) error
  GetDevice() string
  StartServiceBroadcast(timeoutMillis int) error
  StopServiceBroadcast()
  DeviceDiscovery(timeoutMillis int) (string, error)
  RequestServices() (string, error)
  GetServicePrices(serviceID int) (string, error)
  SelectService(serviceID, numberOfUnits, priceID int) (string, error)
  MakePayment(payRequest string) (string, error)
  BeginServiceDelivery(serviceID int, serviceDeliveryToken string, unitsToSupply int) (string, error)
  EndServiceDelivery(serviceID int, serviceDeliveryToken string, unitsReceived int) (string, error)
 // SetEventHandler(handler event.Handler) error
}

func Initialise(name, description string) (Wrapper, error) {
  wpw, err := wpwithin.Initialise(name, description)
  if err != nil {
    return nil, err
  }
  return &WrapperImpl{wpw}, nil
}

type WrapperImpl struct {
  wpw wpwithin.WPWithin
}

func (wrapper *WrapperImpl) AddService(service *types.Service) error {
  return wrapper.wpw.AddService(service)
}

func (wrapper *WrapperImpl) RemoveService(service *types.Service) error {
  return wrapper.wpw.RemoveService(service)
}

func (wrapper *WrapperImpl) InitConsumer(scheme, hostname string, portNumber int, urlPrefix, clientID, hceCard string) error {
  var hceCardObj types.HCECard
  _ = json.Unmarshal([]byte(hceCard), &hceCardObj)
  return wrapper.wpw.InitConsumer(scheme, hostname, portNumber, urlPrefix, clientID, &hceCardObj)
}

func (wrapper *WrapperImpl) InitProducer(merchantClientKey, merchantServiceKey string) error {
  return wrapper.wpw.InitProducer(merchantClientKey, merchantServiceKey);
}

func (wrapper *WrapperImpl) GetDevice() string {
  dev := wrapper.wpw.GetDevice()
  b, _ := json.Marshal(*dev)
  return string(b)
}

func (wrapper *WrapperImpl) StartServiceBroadcast(timeoutMillis int) error {
  return wrapper.wpw.StartServiceBroadcast(timeoutMillis)
}

func (wrapper *WrapperImpl) StopServiceBroadcast() {
  wrapper.wpw.StopServiceBroadcast()
}

func (wrapper *WrapperImpl) DeviceDiscovery(timeoutMillis int) (string, error) {
  bcm, err := wrapper.wpw.DeviceDiscovery(timeoutMillis)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(bcm)
  return string(b), nil
}

func (wrapper *WrapperImpl) RequestServices() (string, error) {
  srv, err := wrapper.wpw.RequestServices()
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(srv)
  return string(b), nil
}

func (wrapper *WrapperImpl) GetServicePrices(serviceID int) (string, error) {
  prc, err := wrapper.wpw.GetServicePrices(serviceID)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(prc)
  return string(b), nil
}

func (wrapper *WrapperImpl) SelectService(serviceID, numberOfUnits, priceID int) (string, error) {
  prc, err := wrapper.wpw.SelectService(serviceID, numberOfUnits, priceID)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(prc)
  return string(b), nil
}

func (wrapper *WrapperImpl) MakePayment(payRequest string) (string, error) {
  var payRequestObj types.TotalPriceResponse
  _ = json.Unmarshal([]byte(payRequest), &payRequestObj)
  rsp, err := wrapper.wpw.MakePayment(payRequestObj)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(rsp)
  return string(b), nil
}

func (wrapper *WrapperImpl) BeginServiceDelivery(serviceID int, serviceDeliveryToken string, unitsToSupply int) (string, error) {
  var serviceDeliveryTokenObj types.ServiceDeliveryToken
  _ = json.Unmarshal([]byte(serviceDeliveryToken), &serviceDeliveryTokenObj)
  tok, err := wrapper.wpw.BeginServiceDelivery(serviceID, serviceDeliveryTokenObj, unitsToSupply)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(tok)
  return string(b), nil
}

func (wrapper *WrapperImpl) EndServiceDelivery(serviceID int, serviceDeliveryToken string, unitsReceived int) (string, error) {
  var serviceDeliveryTokenObj types.ServiceDeliveryToken
  _ = json.Unmarshal([]byte(serviceDeliveryToken), &serviceDeliveryTokenObj)
  tok, err := wrapper.wpw.EndServiceDelivery(serviceID, serviceDeliveryTokenObj, unitsReceived)
  if (err != nil) {
    return "", err
  }
  b, _ := json.Marshal(tok)
  return string(b), nil
}

//func (wrapper *WrapperImpl) SetEventHandler(handler event.Handler) error
