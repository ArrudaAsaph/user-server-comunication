import socket
import json
import psutil

class ComputerInfo:
    """Classe responsável por coletar e armazenar informações do sistema."""

    def __init__(self):
        self.cpu = self.get_num_processors()
        self.ram = self.get_free_ram()
        self.disk = self.get_free_disk_space()
        self.temp = self.get_cpu_temperature()
        self.media = self.calcular_media()

    def get_num_processors(self):
        """Retorna o número de processadores físicos."""
        return psutil.cpu_count(logical=False)

    def get_free_ram(self):
        """Retorna a quantidade de RAM livre em MB."""
        return psutil.virtual_memory().available // (1024 * 1024)

    def get_free_disk_space(self):
        """Retorna o espaço livre em disco em GB."""
        disk = psutil.disk_usage('/')
        return disk.free // (1024 * 1024 * 1024)

    def get_cpu_temperature(self):
        """Retorna a temperatura da CPU em Celsius (se disponível)."""
        try:
            import wmi
            w = wmi.WMI(namespace="root\\wmi")
            temperatures = w.MSAcpi_ThermalZoneTemperature()
            if temperatures:
                return (temperatures[0].CurrentTemperature / 10.0) - 273.15
        except Exception as e:
            print(f"Erro ao obter temperatura: {e}")
        return None 

    def calcular_media(self):
        """Calcula a média simples dos dados."""
        valores = [self.cpu, self.ram, self.disk]
        if self.temp is not None:
            valores.append(self.temp)  
        return sum(valores) / len(valores)

    def formatar_dados(self):
        """Formata os dados do sistema com unidades de medida."""
        return {
            "Processadores": f"{self.cpu} und",
            "RAM Disponível": f"{self.ram} MB",
            "Espaço em Disco Livre": f"{self.disk} GB",
            "Temperatura": f"{self.temp:.2f} °C" if self.temp is not None else "N/A",
            "Média dos Dados": f"{self.media:.2f}"
        }

class Client:
    """Classe responsável pela comunicação com o servidor."""

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.system_info = ComputerInfo()
        self.key = "minhachave"  

    def xor_cipher(self, input_data):
        """Aplica a criptografia XOR aos dados."""
        output_data = bytearray()
        key_len = len(self.key)
        for i in range(len(input_data)):
            output_data.append(input_data[i] ^ ord(self.key[i % key_len]))
        return bytes(output_data)

    def connect(self):
        """Estabelece a conexão com o servidor e envia os dados."""
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            try:
                client_socket.connect((self.host, self.port))
                print(f"🔗 Conectado ao servidor {self.host}:{self.port}")

               
                json_data = json.dumps(self.system_info.formatar_dados()).encode()

                # Criptografa os dados 
                encrypted_data = self.xor_cipher(json_data)
                print(f"🔒 Dados criptografados: {encrypted_data.hex()}")  
                client_socket.send(encrypted_data)
                print("✅ Dados do sistema enviados ao servidor.")

                # Menu de interação
                while True:
                    response = client_socket.recv(1024).decode()
                    if not response:
                        print("⚠️ Servidor desconectou.")
                        break

                    print(response)

                    
                    choice = input("Escolha uma opção: ")
                    client_socket.send(choice.encode())

                    if choice.strip().lower() == "sair":
                        break

            except Exception as e:
                print(f"❌ Erro ao conectar ou comunicar com o servidor: {e}")

if __name__ == "__main__":
    client = Client("192.168.15.14", 8080)
    client.connect()
