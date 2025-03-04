import wmi

def get_cpu_temperature():
    try:
        w = wmi.WMI(namespace="root\\wmi")
        temperatures = w.MSAcpi_ThermalZoneTemperature()
        if temperatures:
            
            return (temperatures[0].CurrentTemperature / 10.0) - 273.15
        return None
    except Exception as e:
        print(f"Erro ao obter temperatura: {e}")
        return None

if __name__ == "__main__":
    temp = get_cpu_temperature()
    if temp is not None:
        print(f"Temperatura da CPU: {temp:.2f}°C")
    else:
        print("Não foi possível obter a temperatura da CPU.")