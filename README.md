# LED-Tree-Arduino
This is the Arduino source code for controlling a Christmas tree covered in Neopixel LED light strips. The project was created for a [Christmas IoT](http://christmasiot.com) popup shop that showcased various uses of IoT technology with a holiday theme.

## Controlling the Tree
The Arduino subscribes to Structure devices messages to control what animation to run and the animation options. The messages should be JSON strings and the various commands are defined below.

### Fade Animation
Causes the tree to fade from a color to another color over a specified duration.

```json
{
  "animation" : "fade",
  "options" : {
    "to" : { "r" : 0, "g" : 0, "b" : 255 },
    "from" : { "r" : 255, "g" : 0, "b" : 0 },
    "duration" : 5000
  }
}
```

The above example animations the tree from solid blue to solid red over 5 seconds.
