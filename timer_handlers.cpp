/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2015 - 2020 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/


#include <QtCore>


void UI_Mainwindow::test_timer_handler()
{
  qDebug() << "scr_update_cntr is: " << waveForm->scr_update_cntr;

  waveForm->scr_update_cntr = 0;
}


void UI_Mainwindow::label_timer_handler()
{
  waveForm->label_active = LABEL_ACTIVE_NONE;
}


void UI_Mainwindow::navDial_timer_handler()
{
  if(navDial->isSliderDown())
  {
    navDialChanged(navDial->value());
  }
  else
  {
    navDial->setSliderPosition(50);

    navDialFunc = NAV_DIAL_FUNC_NONE;

    if(!adjdial_timer->isActive())
    {
      adjDialFunc = ADJ_DIAL_FUNC_NONE;
    }
  }
}


void UI_Mainwindow::adjdial_timer_handler()
{
  adjdial_timer->stop();

  adjDialLabel->setStyleSheet(def_stylesh);

  adjDialLabel->setStyleSheet("font: 7pt;");

  adjDialLabel->setText("");

  if(adjDialFunc == ADJ_DIAL_FUNC_HOLDOFF)
  {
    statusLabel->setText(QString("Trigger holdoff: %1s").arg(suffixed_metric_value(devparms.triggerholdoff, 2)));

    set_cue_cmd(QString(":TRIG:HOLD %1").arg(devparms.triggerholdoff).toLocal8Bit().constData());

    if((devparms.modelserie == 2) || (devparms.modelserie == 6))
    {
      usleep(20000);

      set_cue_cmd(":DISP:CLE");
    }
    else
    {
      usleep(20000);

      set_cue_cmd(":CLE");
    }
  }
  else if(adjDialFunc == ADJ_DIAL_FUNC_ACQ_AVG)
    {
      statusLabel->setText(QString("Acquire averages: %1").arg(devparms.acquireaverages));

      set_cue_cmd(QString(":ACQ:AVER %1").arg(devparms.acquireaverages).toLocal8Bit().constData());
    }

  adjDialFunc = ADJ_DIAL_FUNC_NONE;

  if(navDial_timer->isActive() == false)
  {
    navDialFunc = NAV_DIAL_FUNC_NONE;
  }
}


void UI_Mainwindow::scrn_timer_handler()
{
  if(pthread_mutex_trylock(&devparms.mutexx))
  {
    return;
  }

  scrn_thread->set_params(&devparms);

  scrn_thread->start();
}


void UI_Mainwindow::horPosDial_timer_handler()
{

  QString str;
  if(devparms.timebasedelayenable)
  {
    str = QString(":TIM:DEL:OFFS %1").arg(devparms.timebasedelayoffset);
  }
  else
  {
    str = QString(":TIM:OFFS %1").arg(devparms.timebaseoffset);
  }

  set_cue_cmd(str.toLocal8Bit().constData());
}


void UI_Mainwindow::trigAdjDial_timer_handler()
{
  int chn;

  chn = devparms.triggeredgesource;

  if((chn < 0) || (chn > 3))
  {
    return;
  }

  set_cue_cmd(QString(":TRIG:EDG:LEV %1").arg(devparms.triggeredgelevel[chn]).toLocal8Bit().constData());
}


void UI_Mainwindow::vertOffsDial_timer_handler()
{
  int chn;

  if(devparms.activechannel < 0)
  {
    return;
  }

  chn = devparms.activechannel;

  set_cue_cmd(QString(":CHAN%1:OFFS %2").arg(chn+1).arg(devparms.chanoffset[chn]).toLocal8Bit().constData());
}


void UI_Mainwindow::horScaleDial_timer_handler()
{
  QString str;

  if(devparms.timebasedelayenable)
  {
    str = QString(":TIM:DEL:SCAL %1").arg(devparms.timebasedelayscale);
  }
  else
  {
    str = QString(":TIM:SCAL %1").arg(devparms.timebasescale);
  }

  set_cue_cmd(str.toLocal8Bit().constData());
}


void UI_Mainwindow::vertScaleDial_timer_handler()
{
  int chn;

  if(devparms.activechannel < 0)
  {
    return;
  }

  chn = devparms.activechannel;

  set_cue_cmd(QString(":CHAN%1:SCAL %2").arg(chn + 1).arg(devparms.chanscale[chn]).toLocal8Bit().constData());
}




















