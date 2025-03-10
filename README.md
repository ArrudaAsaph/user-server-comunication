https://www.canva.com/design/DAGhWRMxHts/QOZyYJSdKi3JllD82RmBcg/view?utm_content=DAGhWRMxHts&utm_campaign=designshare&utm_medium=link2&utm_source=uniquelinks&utlId=h8ba091b832#10

### Compilar Servidor em C-> gcc -o Servidor Servidor.c cJSON.c -lws2_32 -Iinclude

#### Rodar -> ./Servidor.exe

# ==============================================

### Para conectar o cliente ao servidor é importante que o servidor esteja permitido a conexão da porta

### Para isso é necessário entrar em *Windows Defender Firewall com Segurança Avançada*
#### Após isso ir em <b> Regras de entrada </b> e criar uma nova Regra 
#### Configurar como regra *porta* após isso clicar em avançar
#### Colocar como regra *TPC* e colocar em portas locais especificas: *8080*
#### E finalizar


### O cliente e o servidor precisam estar na mesma rede wifi!


## Rode primeiro o Servidor com o comando ./Servidor


## Após isso rode o Cliente no Administrador: *Windows PowerShell*
#### (precisa ser no cmd do administrador para permitir a coleta de algumas informações sobre o computador)

