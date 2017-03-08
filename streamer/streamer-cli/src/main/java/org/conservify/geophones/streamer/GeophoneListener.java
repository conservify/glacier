package org.conservify.geophones.streamer;

public class GeophoneListener extends SerialPortLineListener {
    private final GeophoneWriter writer;

    public GeophoneListener(GeophoneWriter writer) {
        this.writer = writer;
    }

    @Override
    public void onLine(String line) {
        String[] parts = line.split(",");
        if (parts.length == 3 || parts.length == 4) {
            boolean success = true;
            float[] values = new float[parts.length];
            for (int i = 0; i < parts.length; ++i) {
                if (parts[i].length() > 0) {
                    values[i] = Float.parseFloat(parts[i]);
                }
                else {
                    success = false;
                }
            }
            if (success) {
                writer.write(values);
            }
        }
    }
}
