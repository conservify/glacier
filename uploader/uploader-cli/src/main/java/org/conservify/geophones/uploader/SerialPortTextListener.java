package org.conservify.geophones.uploader;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CodingErrorAction;

public abstract class SerialPortTextListener {
    private static final int IN_BUFFER_CAPACITY = 128;
    private static final int OUT_BUFFER_CAPACITY = 128;
    private final CharsetDecoder bytesToStrings;
    private final ByteBuffer inFromSerial = ByteBuffer.allocate(IN_BUFFER_CAPACITY);
    private final CharBuffer outToMessage = CharBuffer.allocate(OUT_BUFFER_CAPACITY);

    public SerialPortTextListener() {
        Charset charset = Charset.forName("UTF-8");
        this.bytesToStrings = charset.newDecoder()
                .onMalformedInput(CodingErrorAction.REPLACE)
                .onUnmappableCharacter(CodingErrorAction.REPLACE)
                .replaceWith("\u2e2e");
    }

    public void push(byte[] bytes) {
        int next = 0;
        while (next < bytes.length) {
            while (next < bytes.length && outToMessage.hasRemaining()) {
                int spaceInIn = inFromSerial.remaining();
                int copyNow = bytes.length - next < spaceInIn ? bytes.length - next : spaceInIn;
                inFromSerial.put(bytes, next, copyNow);
                next += copyNow;
                inFromSerial.flip();
                bytesToStrings.decode(inFromSerial, outToMessage, false);
                inFromSerial.compact();
            }
            outToMessage.flip();
            if (outToMessage.hasRemaining()) {
                char[] chars = new char[outToMessage.remaining()];
                outToMessage.get(chars);
                onMessage(chars, chars.length);
            }
            outToMessage.clear();
        }
    }

    public abstract void onMessage(char[] chars, int length);
}
