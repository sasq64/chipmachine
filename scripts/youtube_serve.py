#!/usr/bin/python
import os.path
import flask
app = flask.Flask(__name__)

@app.route("/")
def hello():
    return "Hello World!"

@app.route("/ytmp3/<filename>")
def serve_mp3(filename):
    rname = os.path.join('ytmp3', filename)
    if not os.path.isfile(rname) :
        base,ext = os.path.splitext(filename)
        os.system('youtube-dl -o ytmp3/%s.m4a -x http://www.youtube.com/watch\?v\=%s --audio-format=mp3' % 
          (base, base))
    return flask.send_from_directory('ytmp3', filename)

if __name__ == "__main__":
    app.run(debug = True)

