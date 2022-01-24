So this is a project im working on to replicate the functionality of streamlabs Labels software but with streamelements as a data source (and possibly others while im at it), I also want to make it an obs plugin (yeah spain without the s).


As of right now usage is as such:

apiModule.exe sessions <streamElementsChannelID> <streamElementsJWTToken>

Your channelid is the number next to your twitch account here https://streamelements.com/dashboard/account/channels, and jwtToken is the jwttoken that is revealed by toggling show secrets

Result should be three text files are created in the SE-Labels directory, follower-latest subscriber-latest and tip-latest and populated with the relevant channel data, the console will also print when a new tip sub or follower event occurs and the associated data.

Libraries required to compile
https://github.com/open-source-parsers/jsoncpp
https://github.com/curl/curl

Ill update this readme as I go
