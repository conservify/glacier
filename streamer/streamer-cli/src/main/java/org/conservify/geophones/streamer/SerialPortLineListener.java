package org.conservify.geophones.streamer;

public abstract class SerialPortLineListener extends SerialPortTextListener {
    private final StringBuffer data = new StringBuffer();

    @Override
    public void onMessage(char[] chars, int length) {
        int i = 0;
        int s = 0;
        for (i = 0; i < length; ++i) {
            if (chars[i] == '\r') {
                data.append(chars, s, i - s);
                onLine(data.toString());
                data.setLength(0);
                s = i;
            }
        }

        data.append(chars, s, length - s);
    }

    public abstract void onLine(String line);
}
