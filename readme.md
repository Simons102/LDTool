# Lego Dimensions Tool

A tool that can write and read ntag213 nfc tags for use with Lego Dimensions.

## About

To use this you will need a microcontroller that can do SPI
(I used an arduino uno clone) and an RFID-RC522 reader.

For it to compile you need to have the MFRC522 library installed, which can be obtained through the arduino library manager.

The tags must be ntag213 tags, anything else will not work.  
I used 25mm diameter ones, as they have about the same size
as the tags use by lego and fit nicely inside plastic protective coin cases.  
(That just so happen to have the same outer diameter as the
lego plastic enclosures.)

This tool can do the entire lego dimensions crypto stuff for you, you only have to specify the character/vehicle id.

Most of the crypto heavy lifting was already done [here](https://github.com/phogar/ldnfctags);
this is just an arduino port of the essential stuff.

It takes commands via serial.  
Use the serial monitor and set the baud rate to 9600.

Things that work:
- password generation and authentification
- writing character tags with given id
- writing vehicle tags with given id
- identifying tag -> tag type + character id / vehicle id

Things that don't work:
- vehicle upgrades

## Commands

- `info` - get uid, password and character-/vehicleId (if programmed) from a chip
- `wpwd` - write password
- `wc [characterId]` - write character (password needs to already be written)
- `wpc [characterID]` - write password and character
- `wv [vehicleID]` - write vehicle (password needs to already be written)
- `wpv [vehicleID]` - write password and vehicle
- `calc [characterID]` - calculate encrypted character id for given chip
- `dump` - dump entire chip memory (will attempt to authenticate with password)
- `dumpna` - dump entire chip memory without authenticating with password first

## Character/ vehicle ids

Characters: [characters.txt](characters.txt)

Vehicles 1: [vehicles_1.txt](vehicles_1.txt)

Vehicles 2: [vehicles_2.txt](vehicles_2.txt)

(Taken from children comments of [this](https://www.reddit.com/r/Legodimensions/comments/jlk6ne/which_tags_have_43_pages_in_nfc/gar9tak) reddit comment)

## License

The code is licensed under GPLv3.
