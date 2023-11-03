from media.player import *

start_play = False
def player_event(event,data):
    global start_play
    if(event == K_PLAYER_EVENT_EOF):
        start_play = False

def play_mp4_test(filename):
    global start_play
    player=Player()
    player.load(filename)
    player.set_event_callback(player_event)
    player.start()
    start_play = True

    while(start_play):
        time.sleep(1)


    print("==========palyer stop begin")
    time.sleep(3)
    player.stop()
    print("play over")

play_mp4_test("/sdcard/app/tests/test.mp4")