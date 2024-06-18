import uctypes

k_ai_chn_param_desc = {
    "usr_frame_depth": 0 | uctypes.UINT32,
}

def k_ai_chn_param_parse(s, kwargs):
    s.usr_frame_depth = kwargs.get("usr_frame_depth", 0)

k_ai_chn_pitch_shift_param_desc = {
    "semitones": 0 | uctypes.INT32,
}

def k_ai_chn_pitch_shift_param_parse(s, kwargs):
    s.semitones = kwargs.get("semitones", 0)
