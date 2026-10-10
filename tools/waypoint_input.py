"""Camera-relative virtual-stick inputs for a private waypoint probe.

This inverts the bearing-minus-camera rotation in original SteerTowardStick.
Native routing still needs observation: mathematical tests alone do not prove
collision, turning, or that a building was actually destroyed.
"""
import math


def axes(dx,dz,camera_yaw):
    if not all(math.isfinite(value) for value in (dx,dz,camera_yaw)):
        raise ValueError('Nonfinite waypoint input')
    if dx == dz == 0:return (0,0)
    angle=math.atan2(dx,-dz)+math.radians(camera_yaw%360)
    return round(math.sin(angle)*32767),round(-math.cos(angle)*32767)


def relative_axes(dx,dz,vehicle_heading):
    """Ramdozer LS turns relative to its 4096-unit heading; South accelerates.

    Positive LS X decreases the heading in the observed native Ramdozer flow.
    Other vehicles/control schemes require their own native validation.
    """
    if not all(math.isfinite(value) for value in (dx,dz,vehicle_heading)):
        raise ValueError('Nonfinite relative waypoint input')
    if dx == dz == 0:return (0,0)
    desired=math.atan2(dx,dz)*4096/math.tau
    delta=(desired-vehicle_heading+2048)%4096-2048
    if abs(delta)<=90:return (0,0)
    return (-32767 if delta>0 else 32767),0
