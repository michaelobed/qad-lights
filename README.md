# Quick-and-Dirty Lights

## What is it?

A quick-and-dirty WiFi-configurable RGB LED driver so I can make my wardrobe all pretty inside.

## Why?

I use RGB LED strips a lot, [as you could probably tell](https://github.com/michaelobed/rgb-switcher), and I can't be bothered with (re)programming an Arduino every time I want to change something subtle, so I bashed something together that lets me change everything I would want to remotely. (I say "bashed", but it took me exactly a week. Oh well!)

## Overview

### Concept

The concept is simple: A microcontroller drives three channels of LEDs, one for red, one for green and one for blue. They either stay on permanently, or turn on when switches are activated in different ways. If you want to change how it operates, you access its web portal and fiddle. That's it!

### Why didn't you build rgb-switcher this way?

Because I am a silly-billy who didn't think that through and thought that retroactively adding web configuration using _an entire other daughterboard_ would be a great idea. Maybe I'll do a v2 rearchitected around the ESP32 when life demands less of me. 😂

### The core system

There isn't much to say on this project really, other than that it's based around the ubiquitous ESP32 chip. It has a decent HAL, simple APIs for key-value storage, WiFi and a HTTP server (not HTTPS for this project - more on that later). Add to that the easy-to-configure PWM driver for LED control and you have a browser-controllable RGB LED driver. Tada! 🎉

### PWM

As I mentioned earlier, this project configures 3 GPIOs as individually controllable PWM outputs, with 8-bit resolution per channel for what is effectively 24-bit colour*. The PWM is actually capable of even more resolution than that, but 8-bit is all the discernible colour difference I need I think. The PWM driver even has built-in hardware fade support, so of course I was going to take advantage of that! It wouldn't have been hard to roll my own if it came down to it, like I did in the hitherto hinted at rgb-switcher project, but it came as a nice surprise.

*: In practice, the resolution is a _lot_ less than that, because big RGB LEDs aren't that great at colour accuracy from experience.

### GPIO

The system currently supports two GPIO inputs, representing reed switches which'll go on my wardrobe doors. It'll turn on when either switch is active or when both switches are active, depending on settings. It also supports both normally open and normally closed orientations.

I normally like to set up switch GPIOs to use the internal pull-up of a microcontroller and connect the other end to GND, but for some really weird reason, that doesn't work here, so I've instead pulled them down and tied the other end to +3V3. It feels icky and perverse, but it works at least.

### WiFi

This is the thing the ESP32 really shines at. It has a built-in 2.4GHz WiFi modem, complete with radio circuitry and, depending on which module you use, often has a built-in ~~antenna~~ aerial 🇬🇧. By default, it boots into access point mode, meaning you connect to _it_ to access its web portal. You can later configure it to connect to your own network for easier access if you so desire.

### HTTP server

This is easily the thing that took the longest, partly because PWM and WiFi are easy and because I really don't do any web coding! I used the actually rather good built-in ESP HTTP Server library, slapped some pages together in HTML and Javascript, then added Websockets support for good measure, so that config changes and RGB sliders all work in realtime.

I almost forgot to mention the lack of HTTP**S** support: I believe you need to supply your own .pem certificate to use that, and for this simple use case that will never see the light of the Internet, I couldn't be bothered with that. It's a good thing to consider, though, especially given that the password to your WiFi network of choice is otherwise supplied over an unencrypted connection in plaintext using the HTTP POST method. A man-in-the-middle attack is possible I suppose, but improbable and impractical, so I'm not too worried.

## Final notes

I should point out that I developed this against an ESP-WROOM-32 module sitting on an ESP32-DevKitC board, which is probably really old! I'm sure the code is device-agnostic enough to easily get working on other modules, or even a custom board, but I've done no testing on this. Your mileage may vary, I guess.

I've included a very rough schematic as a guide on how I put this together. I didn't really use a 5V voltage regulator in practice since the dev board was powered via USB for ease of debugging. I pretty much copied the power circuitry from rgb-switcher, and I see no reason for it not to work here given the similar power domain requirements.

I did have a weird issue where the red LED was quite a bit dimmer than green and blue, even after swapping GPIOs around a bit. This ended up being the use of IRF540 mosfets in the end - they barely conduct with the gate-source voltage of 3.3V. I presume the difference in forward voltage of green/blue LEDs vs. red LEDs also contributed to this. I swapped them out for TIP41C darlington BJTs, which I've also used in the past to drive RGB LED strips, though they waste a bit more power doing it from my experience. The fades between the channels are also no longer uniform annoyingly, but at least full white looks white now instead of blue! I didn't get to try the IRF540s with other LED strips, but I'm sure this is a more universal solution than what I had.

As always, I'm releasing this under the MIT license so people can fork the repo, modify the code, sell a product based around it or whatever else. I'm just happy this exists and I can stop writing an entire LED driver when I need one!

## What next?

Since I did bash this together, there are a number of things that are missing or that I'm not completely happy with (in no particular order):

- **Over-the-air firmware update support**. I've provided a partition or two for that, but I've never looked into doing it on the ESP32. It seems simple enough to do, however. Maybe one of you can do it... 😏
- **Support for a variable number of switches**. In theory, the logic I have in place makes that feasible, and the `Config` class can easily be extended to allow both the number of switches and their GPIO assignments to be configurable.
- **Make the UI less boring**? This one's more of a maybe since I don't really care that much and just wanted it working, but perhaps a square that changes colours next to the RGB sliders could be cool, albeit more colour accurate than the lights will ever be.
- **Sleep mode**! These should be able to go to sleep when sitting there for ages doing nothing, then wake up and drive the LEDs when the door is opened. This should be easy enough to set up as a wake-up interrupt of some kind.
- **Configurable fade time**. This was actually supposed to be in the `Config` class too, but I just haven't gotten around to it.
