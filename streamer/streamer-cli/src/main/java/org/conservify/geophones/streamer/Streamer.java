package org.conservify.geophones.streamer;

import jssc.SerialPort;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Streamer implements SerialPortEventListener {
    private static final Logger logger = LoggerFactory.getLogger(Streamer.class);
    private final SerialPort port;
    private final SerialPortTextListener listener;
    private final int baudRate = 115200;

    private volatile long firstActivityAt = 0;
    private volatile long lastActivityAt = 0;

    private static final long ACTIVITY_TIMEOUT = 1000 * 5;

    public Streamer(SerialPort port, SerialPortTextListener listener) {
        this.port = port;
        this.listener = listener;
    }

    public boolean start() {
        try {
            if (port.isOpened()) {
                port.closePort();
            }
            port.openPort();
            port.addEventListener(this);
            port.purgePort(SerialPort.PURGE_RXCLEAR);
            port.purgePort(SerialPort.PURGE_TXCLEAR);
            port.setParams(baudRate, 8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
            return true;
        }
        catch (SerialPortException e) {
            logger.error(e.getMessage(), e);
            return false;
        }
    }

    public boolean check() {
        if (System.currentTimeMillis() - lastActivityAt > ACTIVITY_TIMEOUT) {
            if (lastActivityAt > 0) {
                logger.info("Re-opening {}", port.getPortName());
            }
            stop();
            return false;
        }
        return true;
    }

    public void stop() {
        try {
            this.port.closePort();
        }
        catch (SerialPortException e) {
        }
    }

    @Override
    public void serialEvent(SerialPortEvent serialPortEvent) {
        if (serialPortEvent.isRXCHAR()) {
            try {
                listener.push(port.readBytes(serialPortEvent.getEventValue()));
            } catch (SerialPortException e) {
                logger.error("Error", e);
            }

            lastActivityAt = System.currentTimeMillis();

            if (firstActivityAt == 0) {
                firstActivityAt = System.currentTimeMillis();
            }
        }
        else {
            System.out.println(serialPortEvent.getEventType());
        }
    }
}
