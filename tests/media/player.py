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
        time.sleep(0.1)

    player.stop()
    print("play over")

'''
def test(filename):
    player=Player()
    print("....load_mp4")
    player.load(filename)
    time.sleep(1)
    print("....destroy_mp4")
    player.destroy_mp4();


    print("======_init_media_buffer before")
    time.sleep(1)
    player._init_media_buffer()
    print("======_init_media_buffer end")
    time.sleep(3)
    print("======_deinit_media_buffer before")
    player._deinit_media_buffer()
    print("======_deinit_media_buffer end")
'''

play_mp4_test("/sdcard/app/tests/test.mp4")