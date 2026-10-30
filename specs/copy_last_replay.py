import obspython as obs
import os
import shutil

def script_description():
    return "Автоматично копира последния записан Replay Buffer файл в нов файл с фиксирано име 'last_replay'."

def get_last_replay_path():
    """ Извлича пълния път до последния записан клип от Replay Buffer на OBS """
    try:
        replay_buffer = obs.obs_frontend_get_replay_buffer_output()
        if not replay_buffer:
            return None
        
        cd = obs.calldata_create()
        ph = obs.obs_output_get_proc_handler(replay_buffer)
        obs.proc_handler_call(ph, "get_last_replay", cd)
        path = obs.calldata_string(cd, "path")
        
        obs.calldata_destroy(cd)
        obs.obs_output_release(replay_buffer)
        return path
    except Exception as e:
        obs.script_log(obs.LOG_ERROR, f"Грешка при взимане на пътя на клипа: {str(e)}")
        return None

def on_event(event):
    # Улавя събитието, че файлът от Replay Buffer е успешно записан на диска
    if event == obs.OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:
        obs.script_log(obs.LOG_INFO, "Засечен е нов записан клип. Стартиране на копирането...")
        
        # Взимаме пътя на оригиналния последен клип
        source_path = get_last_replay_path()
        
        if source_path and os.path.isfile(source_path):
            # Взимаме папката на оригиналния файл и неговото разширение (.mp4, .mkv и т.н.)
            folder = os.path.dirname(source_path)
            filename, ext = os.path.splitext(source_path)
            
            # Дефинираме новия път (записва се в същата папка като 'last_replay.mp4' / 'last_replay.mkv')
            destination_path = os.path.join(folder, f"last_replay{ext}")
            
            try:
                # Копираме оригиналния файл върху фиксирания файл (ако съществува стар, ще го презапише)
                shutil.copy2(source_path, destination_path)
                obs.script_log(obs.LOG_INFO, f"Успешно копиране!\nНов файл: {destination_path}")
            except Exception as e:
                obs.script_log(obs.LOG_ERROR, f"Грешка при копиране на файла: {str(e)}")
        else:
            obs.script_log(obs.LOG_WARNING, "Не беше намерен валиден път до последния клип.")

def script_load(settings):
    # Свързваме функцията on_event със събитията на OBS
    obs.obs_frontend_add_event_callback(on_event)
    obs.script_log(obs.LOG_INFO, "Скриптът за копиране на Replays е зареден успешно.")
