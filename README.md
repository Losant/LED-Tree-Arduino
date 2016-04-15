# Arduino Powered LED Christmas Tree
This is the Arduino source code for controlling a Christmas tree covered in Neopixel LED light strips. The project was created for a [Christmas IoT](http://christmasiot.com) popup shop that showcased various uses of IoT technology with a holiday theme. The Arduino connects to the [Losant IoT platform](https://www.losant.com) in order to receive commands. If you're interested in the actual components used to build the tree, please check out the related [blog article](https://www.losant.com/blog/arduino-powered-led-christmas-tree).

<p style="text-align:center;">

<img src="https://raw.githubusercontent.com/Losant/LED-Tree-Arduino/master/readme-assets/warpcore.gif">

<img src="https://raw.githubusercontent.com/Losant/LED-Tree-Arduino/master/readme-assets/fireworks.gif">

<img src="https://raw.githubusercontent.com/Losant/LED-Tree-Arduino/master/readme-assets/fade.gif">
</p>

## About Losant
[Losant](https://www.losant.com) is an IoT platform that is currently free to use for a limited number of devices. Losant is used to facilitate communication between the [tree's website](http://led-tree.christmasiot.com) and the Arduino via MQTT. If you're interesting in learning more about Losant and keeping up-to-date with product announcements, please follow us at [@LosantHQ](https://twitter.com/LosantHQ).

## Controlling the Tree
The Arduino subscribes to Losant device messages to control what animation to run and the animation options. The messages should be JSON strings and the various commands are defined below.

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

### Warp Core Animation
Runs an animation that kind of looks like the warp core from Star Trek: the Next Generation.

```json
{
  "animation" : "warpcore",
  "options" : {
    "duration" : 15000
  }
}
```

The above example will run the warp core animation for 15 seconds.

### Firework Animations
Runs an animation that looks like a firework.

```json
{
  "animation" : "firework",
  "options" : {
    "duration" : 15000
  }
}
```

The above example will run the firework animation for 15 seconds.

## Controlling Tree with Losant Webhook
The tree can also be controlled by POSTing JSON data to the Losant webhook URL. The webhook triggers a workflow that simply passes the data directly to the device using the above mechanism. The same payloads apply.

```
POST: https://triggers.losant.com/webhooks/zzplbKb8yJH6ajWWdGL6HOKxndm1
```

## Controlling the Tree with Website
We created a website at [http://led-tree.christmasiot.com](http://led-tree.christmasiot.com) that can be used to control the tree. The website also live-streams the tree so you can see your changes as well as what others are doing to the tree.

<p style="text-align:center;">
<img src="https://raw.githubusercontent.com/Losant/LED-Tree-Arduino/master/readme-assets/website.png">
</p>
