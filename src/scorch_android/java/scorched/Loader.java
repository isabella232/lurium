package scorched;

import com.anders_e.lurium.MainLoop;

public class Loader extends MainLoop {
    static {
        System.loadLibrary("scorched");
    }
}
