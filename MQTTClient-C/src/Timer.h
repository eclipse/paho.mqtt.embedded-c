#ifndef MQTT_CLIENT_C_TIMER_H
#define MQTT_CLIENT_C_TIMER_H

void *TimerInit();
char TimerIsExpired(void *timer);
void TimerCountdownMS(void *timer, unsigned int);
void TimerCountdown(void *timer, unsigned int);
int TimerLeftMS(void *timer);
void destroyTimer(void *t);

#endif
