Respuestas conceptuales

Nombre y apellido: Daniela Pardo
Legajo: 45524660
1. TCP y los límites de los mensajes

¿Por qué TCP no conserva los límites entre los mensajes enviados por la aplicación?

Respuesta:
Por que el protocolo esta orientado a bytes, un flujo continuo de datos. No a mensajes. La aplicacion establece un acuerdo previo entre el emisor y el receptor en la que (\n) delimita el "mensaje" y su fin.

2. Recepción de una línea

¿Por qué no es correcto suponer que una llamada a recv() devolverá una línea completa?

Respuesta: Porque recv() devuelve lo que haya en el buffer del socket en ese momento hasta el tamaño pedido. 



3. Descriptores del servidor

¿Qué diferencia existe entre el descriptor retornado por socket() antes de listen() y el descriptor retornado por accept()?

Respuesta:

El socket() cuando se le aplica bind y listen, escucha. No transmite datos y solo acepta conexiones.
El socket de accept () devuelve un socket activo que representa a la conexion establecida con un cliente en la que si se pueden intercambiar datos con recv() y send().

4. Cierre ordenado

¿Qué significa que recv() retorne cero?

Respuesta:
Significa que el otro extremo envio FIN.

5. Envíos parciales

¿Por qué send() puede enviar menos bytes que los solicitados?

Respuesta: 
Porque el buffer de envio puede estar lleno o la llamada puede ser interrumpida por una señal. 

6. Reutilización de la dirección

¿Qué problema evita SO_REUSEADDR durante las pruebas del servidor?

Respuesta:
Permite que bind() tenga exitoen estado de TIME_WAIT tras cerrar el servidor, permitiendo reusar el puerto inmediatamente.

7. Ciclo de atención

¿Por qué el servidor vuelve a accept() después de procesar QUIT?

Respuesta:

Porque QUIT cierra la conexion actual, pero no el servidor. El socket de escucha sigue abierto y el servidor esta disponible para el proximo cliente. 